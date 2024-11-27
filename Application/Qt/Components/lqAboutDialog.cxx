/*=========================================================================

  Program:   LidarView
  Module:    lqAboutDialog.cxx

  Copyright (c) Kitware, Inc.
  All rights reserved.
  See LICENSE or http://www.apache.org/licenses/LICENSE-2.0 for details.

  This software is distributed WITHOUT ANY WARRANTY; without even
  the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
  PURPOSE.  See the above copyright notice for more information.

=========================================================================*/

#include "lqAboutDialog.h"
#include "ui_lqAboutDialog.h"

#include <tins/tins.h>

#include <vtkLVVersion.h>

#include <QtTestingConfigure.h>
#include <pqApplicationCore.h>
#include <pqCoreUtilities.h>
#include <pqFileDialog.h>
#include <vtkNew.h>
#include <vtkPVOpenGLInformation.h>
#include <vtkPVPythonInformation.h>
#include <vtkPVVersion.h>
#include <vtkProcessModule.h>
#include <vtkSMPTools.h>
#include <vtkSMProxyManager.h>
#include <vtkSMSession.h>
#include <vtkSMViewProxy.h>
#include <vtkVersion.h>

#include <QApplication>
#include <QClipboard>
#include <QFile>
#include <QHeaderView>
#include <QOpenGLContext>
#include <QOpenGLFunctions>

#include <sstream>

//-----------------------------------------------------------------------------
lqAboutDialog::lqAboutDialog(QWidget* Parent)
  : QDialog(Parent)
  , Ui(new Ui::lqAboutDialog())
{
  this->Ui->setupUi(this);
  this->setObjectName("lqAboutDialog");
  // hide the Context Help item (it's a "?" in the Title Bar for Windows, a menu item for Linux)
  this->setWindowFlags(this->windowFlags().setFlag(Qt::WindowContextHelpButtonHint, false));

  QString spashImage = QString(":/%1/SplashImage.img").arg(QApplication::applicationName());
  if (QFile::exists(spashImage))
  {
    this->Ui->Image->setPixmap(QPixmap(spashImage));
  }

  // get extra information and put it in
  this->Ui->VersionLabel->setText(QString("<html><b>%1: <i>%2</i></b></html>")
                                    .arg(tr("Version"))
                                    .arg(QString(LIDARVIEW_VERSION_FULL)));
  this->AddInformationPanel();
  this->AddClientInformation();
}

//-----------------------------------------------------------------------------
lqAboutDialog::~lqAboutDialog()
{
  delete this->Ui;
}

//-----------------------------------------------------------------------------
inline void addItem(QTreeWidget* tree, const QString& key, const QString& value)
{
  QTreeWidgetItem* item = new QTreeWidgetItem(tree);
  item->setText(0, key);
  item->setText(1, value);
}
inline void addItem(QTreeWidget* tree, const QString& key, int value)
{
  ::addItem(tree, key, QString("%1").arg(value));
}

//-----------------------------------------------------------------------------
void lqAboutDialog::AddClientInformation()
{

  QTreeWidget* tree = this->Ui->ClientInformation;

  ::addItem(tree, tr("LidarView Version"), QString(LIDARVIEW_VERSION_FULL));
  ::addItem(tree, tr("ParaView Version"), QString(PARAVIEW_VERSION_FULL));
  ::addItem(tree, tr("VTK Version"), QString(vtkVersion::GetVTKVersionFull()));
  ::addItem(tree, tr("Qt Version"), QT_VERSION_STR);
  ::addItem(tree, tr("Libpcap Version"), pcap_lib_version());

  ::addItem(tree, tr("vtkIdType size"), tr("%1bits").arg(8 * sizeof(vtkIdType)));

  vtkNew<vtkPVPythonInformation> pythonInfo;
  pythonInfo->CopyFromObject(nullptr);

  ::addItem(tree, tr("Embedded Python"), pythonInfo->GetPythonSupport() ? tr("On") : tr("Off"));
  if (pythonInfo->GetPythonSupport())
  {
    ::addItem(tree, tr("Python Library Path"), QString::fromStdString(pythonInfo->GetPythonPath()));
    ::addItem(
      tree, tr("Python Library Version"), QString::fromStdString(pythonInfo->GetPythonVersion()));

    ::addItem(
      tree, tr("Python Numpy Support"), pythonInfo->GetNumpySupport() ? tr("On") : tr("Off"));
    if (pythonInfo->GetNumpySupport())
    {
      ::addItem(tree, tr("Python Numpy Path"), QString::fromStdString(pythonInfo->GetNumpyPath()));
      ::addItem(
        tree, tr("Python Numpy Version"), QString::fromStdString(pythonInfo->GetNumpyVersion()));
    }
  }

#if defined(QT_TESTING_WITH_PYTHON)
  ::addItem(tree, tr("Python Testing"), tr("On"));
#else
  ::addItem(tree, tr("Python Testing"), tr("Off"));
#endif

#if VTK_MODULE_ENABLE_VTK_ParallelMPI
  ::addItem(tree, tr("MPI Enabled"), tr("On"));
#else
  ::addItem(tree, tr("MPI Enabled"), tr("Off"));
#endif

  ::addItem(tree, tr("SMP Backend"), vtkSMPTools::GetBackend());
  ::addItem(tree, tr("SMP Max Number of Threads"), vtkSMPTools::GetEstimatedNumberOfThreads());

  // For local OpenGL info, we ask Qt, as that's more truthful anyways.
  QOpenGLContext* ctx = QOpenGLContext::currentContext();
  if (QOpenGLFunctions* f = ctx ? ctx->functions() : nullptr)
  {
    const char* glVendor = reinterpret_cast<const char*>(f->glGetString(GL_VENDOR));
    const char* glRenderer = reinterpret_cast<const char*>(f->glGetString(GL_RENDERER));
    const char* glVersion = reinterpret_cast<const char*>(f->glGetString(GL_VERSION));
    ::addItem(tree, tr("OpenGL Vendor"), glVendor);
    ::addItem(tree, tr("OpenGL Version"), glVersion);
    ::addItem(tree, tr("OpenGL Renderer"), glRenderer);
  }

  tree->header()->setSectionResizeMode(QHeaderView::ResizeToContents);
}

//-----------------------------------------------------------------------------
void lqAboutDialog::AddInformationPanel()
{
  auto text = QString(" \
    Kitware is a leading provider of open-source software solutions for scientific computing and computer Vision. <br /> <br /> \
    We are the developers of LidarView, providing real-time interactive visualization of live captured 3D LiDAR \
    data from Lidar sensors, replay of this data and apply many types of algorithm on it to extract meaningful \
    information from it. <br /> <br /> \
    We apply our computer vision expertise to create customized solutions using 3D algorithm and machine learning to provide \
    solutions in many applications such as detection and tracking of people, traffic signs, lane markings, vehicles, \
    industrial machinery from within LidarView ( and in ROS or custom application), using combinations of point cloud and \
    extra sensors (camera, IMU, GPS...). <br /> \
    We also provide Lidar-based SLAM algorithms for localizing the sensor at any time, and building high fidelity maps. <br /> <br /> \
    We work with customers to create tailored solutions using proven open-source technologies, avoiding vendor lock-in and \
    leveraging our world-leading experience in visualization, computer vision, high-performance computing, and test-driven \
    high-quality software process. <br /> <br /> \
    Have a look at <a href=\"https://www.kitware.com/our-expertise/ \">our expertise</a> to see other open-source tools \
    that we develop, and for more information, please contact us: \
    <a href=\"mailto:kitware@kitware.fr?subject=Contact+about+LidarView\">kitware@kitware.fr</a> \
");

  this->Ui->MoreInfoPanel->setText(text);
}

//-----------------------------------------------------------------------------
QString lqAboutDialog::formatToText(QTreeWidget* tree)
{
  QString text;
  QTreeWidgetItemIterator it(tree);
  while (*it)
  {
    text += (*it)->text(0) + ": " + (*it)->text(1) + "\n";
    ++it;
  }
  return text;
}

//-----------------------------------------------------------------------------
QString lqAboutDialog::formatToText()
{
  QString text = tr("Client Information:\n");
  QTreeWidget* tree = this->Ui->ClientInformation;
  text += this->formatToText(tree);
  return text;
}

//-----------------------------------------------------------------------------
void lqAboutDialog::saveToFile()
{
  pqFileDialog fileDialog(nullptr,
    pqCoreUtilities::mainWidget(),
    tr("Save to File"),
    QString(),
    tr("Text Files") + " (*.txt);;" + tr("All Files") + " (*)",
    false);
  fileDialog.setFileMode(pqFileDialog::AnyFile);
  if (fileDialog.exec() != pqFileDialog::Accepted)
  {
    // Canceled
    return;
  }

  QString filename = fileDialog.getSelectedFiles().first();
  QByteArray filename_ba = filename.toUtf8();
  std::ofstream fileStream;
  fileStream.open(filename_ba.data());
  if (fileStream.is_open())
  {
    fileStream << this->formatToText().toStdString();
    fileStream.close();
  }
}

//-----------------------------------------------------------------------------
void lqAboutDialog::copyToClipboard()
{
  QClipboard* clipboard = QGuiApplication::clipboard();
  clipboard->setText(this->formatToText());
}
