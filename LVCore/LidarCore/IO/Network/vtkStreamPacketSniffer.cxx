/*=========================================================================

  Program: LidarView
  Module:  vtkStreamPacketSniffer.cxx

  Copyright (c) Kitware Inc.
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

  This software is distributed WITHOUT ANY WARRANTY; without even
  the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
  PURPOSE.  See the above copyright notice for more information.

=========================================================================*/

#include "vtkStreamPacketSniffer.h"

#include "vtkPacketRecorder.h"
#include "vtkSynchronizedQueue.h"

#include <vtkLogger.h>
#include <vtkNew.h>
#include <vtkObjectFactory.h>

#include <tins/tins.h>

#include <atomic>
#include <ctime>
#include <memory>
#include <sstream>
#include <thread>

namespace
{
constexpr unsigned int PKT_CACHE_SIZE = 500;

//-----------------------------------------------------------------------------
class PacketSnifferImpl
{
public:
  using HandleReceiveCallback = std::function<bool(Tins::Packet)>;

  //-----------------------------------------------------------------------------
  PacketSnifferImpl(std::string interfaceName,
    std::string filter,
    HandleReceiveCallback addToQueueCallback)
    : AddToQueueCallback(addToQueueCallback)
  {
    Tins::SnifferConfiguration config;
    // config.set_promisc_mode(true); // Enable promiscuous mode to capture all packets
    config.set_filter(filter);
    try
    {
      this->Sniffer = std::make_unique<Tins::Sniffer>(interfaceName, config);
    }
    catch (const std::exception& ex)
    {
      std::stringstream errorMessage;
      errorMessage << "Error: " << ex.what() << ".";
      if (std::strcmp(ex.what(), "socket: Operation not permitted") == 0)
      {
        errorMessage << " Please check you have sufficient permission.";
      }
      vtkWarningWithObjectMacro(nullptr, << errorMessage.str());
      this->Sniffer.reset();
    }
  }

  //-----------------------------------------------------------------------------
  bool StartCapture()
  {
    bool started = false;
    if (!this->Sniffer)
    {
      return started;
    }

    std::mutex mtx;
    std::condition_variable cond;

    this->Running = true;
    this->ReceiverThread = std::thread(
      [&]()
      {
        {
          std::lock_guard<std::mutex> _(mtx);
          started = true;
          cond.notify_one();
        }
        std::function<bool(Tins::PDU&)> functor = std::bind(&PacketSnifferImpl::Callback, this, std::placeholders::_1);
        this->Sniffer->sniff_loop(functor);
      });

    std::unique_lock<std::mutex> locker(mtx);
    while (!started)
    {
      cond.wait(locker);
    }
    return started;
  }

  //-----------------------------------------------------------------------------
  void StopCapture()
  {
    this->Running = false;
    this->Sniffer->stop_sniff();
    this->ReceiverThread.join();
  }

  //-----------------------------------------------------------------------------
  bool Callback(const Tins::PDU& pdu)
  {
    const Tins::RawPDU* raw = pdu.find_pdu<Tins::RawPDU>();
    if (raw)
    {
      // Craft a packet with current time timestamp
      Tins::Packet packet(pdu);
      this->AddToQueueCallback(packet);
    }
    return this->Running;
  }

  static std::vector<std::string> GetAllInterfaces()
  {
    std::vector<std::string> interfaces;
    std::vector<Tins::NetworkInterface> interfacesToListen;
    for (auto &tinsInterfaces : Tins::NetworkInterface::all())
    {
      interfaces.emplace_back(tinsInterfaces.name());
    }
#if defined(_WIN32)
    interfaces.emplace_back("Loopback");
#endif
    return interfaces;
  }

private:
  std::thread ReceiverThread;
  std::unique_ptr<Tins::Sniffer> Sniffer;
  std::atomic<bool> Running = false;
  HandleReceiveCallback AddToQueueCallback;
};

using QueueType = vtkSynchronizedQueue<Tins::Packet>;
}

//-----------------------------------------------------------------------------
vtkStandardNewMacro(vtkStreamPacketSniffer);

//-----------------------------------------------------------------------------
class vtkStreamPacketSniffer::vtkInternals
{
public:
  std::unique_ptr<QueueType> DataQueue;
  ConsumeCallback Callback;

  std::vector<std::unique_ptr<PacketSnifferImpl>> Sniffers;

  vtkPacketRecorder* Writer;

  //------------------------------------------------------------------------------
  void StartSniffers(std::string networkInteface, std::string filter)
  {
    this->DataQueue = std::make_unique<QueueType>(::PKT_CACHE_SIZE);

    std::vector<std::string> interfacesToListen;
    if (!networkInteface.empty())
    {
      interfacesToListen.emplace_back(networkInteface);
    }
    else
    {
      interfacesToListen = PacketSnifferImpl::GetAllInterfaces();
    }

    for (auto& netInterface : interfacesToListen)
    {
      auto callback = std::bind(&QueueType::Enqueue, this->DataQueue.get(), std::placeholders::_1);
      auto sniffer = std::make_unique<PacketSnifferImpl>(netInterface, filter, callback);
      if (sniffer->StartCapture())
      {
        this->Sniffers.emplace_back(std::move(sniffer));
      }
    }
  }

  //------------------------------------------------------------------------------
  void ConsumeLoop()
  {
    Tins::Packet packet;
    while (this->DataQueue->Dequeue(packet))
    {
      const Tins::PDU* pdu = packet.pdu();
      const Tins::RawPDU* raw = pdu->find_pdu<Tins::RawPDU>();

      Tins::Timestamp timestamp = packet.timestamp();
      double timeSec = timestamp.seconds() + timestamp.microseconds() * 1e-6;
      this->Callback(raw->payload(), timeSec);

      if (this->Writer && this->Writer->GetIsRecording())
      {
        this->Writer->AddPacketToWritingQueue(&packet);
      }
    }
  }
};

//------------------------------------------------------------------------------
vtkStreamPacketSniffer::vtkStreamPacketSniffer()
  : Internals(new vtkStreamPacketSniffer::vtkInternals())
{
}

//------------------------------------------------------------------------------
vtkStreamPacketSniffer::~vtkStreamPacketSniffer()
{
  this->StopListening();
}

//----------------------------------------------------------------------------
bool vtkStreamPacketSniffer::StartListening(const std::vector<unsigned int>& ports,
  const ConsumeCallback& callback)
{
  const std::string libVersion = pcap_lib_version();
  if (libVersion.find("WinPcap") != std::string::npos)
  {
    vtkWarningMacro(<< "Cannot sniff packets with WinPcap please use Npcap instead.");
    return false;
  }
  if (this->IsListening())
  {
    // Stop the stream, to handle succeeding call to Start()
    this->StopListening();
  }

  auto& internals = *this->Internals;
  internals.Callback = callback;

  std::string filter = "udp ";
  if (!this->HostAddress.empty())
  {
    filter += " src host " + this->HostAddress + " ";
  }
  for (size_t idx = 0; idx < ports.size(); idx++)
  {
    unsigned int port = ports.at(idx);
    filter.append("port " + std::to_string(port));
    if (idx < ports.size() - 1)
    {
      filter.append(" or ");
    }
  }

  internals.StartSniffers(this->NetworkInterface, filter);
  if (!internals.Sniffers.empty())
  {
    this->ConsumerThread = std::make_unique<std::thread>([&internals] { internals.ConsumeLoop(); });
  }
  return true;
}

//----------------------------------------------------------------------------
void vtkStreamPacketSniffer::StopListening()
{
  auto& internals = *this->Internals;
  for (auto& sniffer : internals.Sniffers)
  {
    sniffer->StopCapture();
  }
  internals.Sniffers.clear();
  if (internals.DataQueue)
  {
    internals.DataQueue->StopQueue();
  }
  if (this->ConsumerThread && this->ConsumerThread->joinable())
  {
    this->ConsumerThread->join();
  }
  this->ConsumerThread.reset();
  internals.DataQueue.reset();
}

//-----------------------------------------------------------------------------
bool vtkStreamPacketSniffer::IsListening()
{
  return !this->Internals->Sniffers.empty();
}

//-----------------------------------------------------------------------------
void vtkStreamPacketSniffer::SetRecorder(vtkPacketRecorder* writer)
{
  this->Internals->Writer = writer;
}
