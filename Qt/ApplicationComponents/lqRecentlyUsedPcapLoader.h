/*=========================================================================

  Program: LidarView
  Module:  lqRecentlyUsedPcapLoader.h

  Copyright (c) Kitware Inc.
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

  This software is distributed WITHOUT ANY WARRANTY; without even
  the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
  PURPOSE.  See the above copyright notice for more information.

=========================================================================*/

#ifndef lqRecentlyUsedPcapLoader_h
#define lqRecentlyUsedPcapLoader_h

#include <pqRecentlyUsedResourceLoaderInterface.h>

#include "lqApplicationComponentsModule.h" // needed for export macros
#include <QObject>                         // needed for QObject
#include <QStringList>                     // needed for QStringList

#include <vtkSMProxy.h>

/**
 * @class lqRecentlyUsedPcapLoader
 * @brief support loading pcap data files when loaded from lqRecentPcapFilesMenu.
 *
 * lqRecentlyUsedPcapLoader is an implementation of the
 * pqRecentlyUsedResourceLoaderInterface that support loading specifically pcap files.
 */
class LQAPPLICATIONCOMPONENTS_EXPORT lqRecentlyUsedPcapLoader
  : public QObject
  , public pqRecentlyUsedResourceLoaderInterface
{
  Q_OBJECT
  Q_INTERFACES(pqRecentlyUsedResourceLoaderInterface)

  typedef QObject Superclass;

public:
  lqRecentlyUsedPcapLoader(QObject* parent = nullptr);
  ~lqRecentlyUsedPcapLoader() override;

  bool canLoad(const pqServerResource& resource) override;
  bool load(const pqServerResource& resource, pqServer* server) override;
  QIcon icon(const pqServerResource& resource) override;
  QString label(const pqServerResource& resource) override;

  /**
   * Add data files(s) to the recently used resources list.
   */
  static bool addPcapFileToRecentResources(pqServer* server,
    const QString& file,
    vtkSMProxy* proxy);

private:
  Q_DISABLE_COPY(lqRecentlyUsedPcapLoader)

  bool loadPcapData(const pqServerResource& resource, pqServer* server);
};

#endif
