/*=========================================================================

  Program: LidarView
  Module:  vtkTCPPacketSender.h

  Copyright (c) Kitware Inc.
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

  This software is distributed WITHOUT ANY WARRANTY; without even
  the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
  PURPOSE.  See the above copyright notice for more information.

=========================================================================*/

#ifndef vtkTCPPacketSender_h
#define vtkTCPPacketSender_h

#include <vtkObject.h>
#include <vtkSmartPointer.h>

#include "lvIONetworkModule.h"

#include <memory>
#include <string>

class vtkPacketFileHandler;

class LVIONETWORK_EXPORT vtkTCPPacketSender : public vtkObject
{
public:
  vtkTypeMacro(vtkTCPPacketSender, vtkObject);
  static vtkTCPPacketSender* New();

  ///@{
  /**
   * Set the PCAP file name.
   */
  vtkGetMacro(PCAPFileName, std::string);
  vtkSetMacro(PCAPFileName, std::string);
  ///@}

  ///@{
  /**
   * Address on which the client will try to connect with TCP
   */
  vtkGetMacro(Address, std::string);
  vtkSetMacro(Address, std::string);
  ///@}

  ///@{
  /**
   * Port
   */
  vtkGetMacro(Port, int);
  vtkSetMacro(Port, int);
  ///@}

  /**
   *
   */
  bool SendAllPackets(double speed = 1.0);

protected:
  vtkTCPPacketSender();

private:
  double PumpPacket();

private:
  class vtkInternals;
  std::unique_ptr<vtkInternals> Internals;

  std::string PCAPFileName;
  std::string Address;
  int Port;

  vtkSmartPointer<vtkPacketFileHandler> PacketReader;
};

#endif
