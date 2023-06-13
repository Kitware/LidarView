/*=========================================================================

   Program: LidarView
   Module:  lqLidarViewManager.h

   Copyright (c) Kitware Inc.
   All rights reserved.

   LidarView is a free software; you can redistribute it and/or modify it
   under the terms of the LidarView license.

   See LICENSE for the full LidarView license.
   A copy of this license can be obtained by contacting
   Kitware Inc.
   28 Corporate Drive
   Clifton Park, NY 12065
   USA

THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS
``AS IS'' AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT
LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR
A PARTICULAR PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL THE AUTHORS OR
CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL,
EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO,
PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR
PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF
LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING
NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS
SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.

========================================================================*/
#ifndef LQLIDARVIEWMANAGER_H
#define LQLIDARVIEWMANAGER_H

#include <lqLidarCoreManager.h>

#include "lvApplicationComponentsModule.h"

class LVAPPLICATIONCOMPONENTS_EXPORT lqLidarViewManager : public lqLidarCoreManager
{

  Q_OBJECT
  typedef lqLidarCoreManager Superclass;

public:
  lqLidarViewManager(QObject* parent = nullptr);
  ~lqLidarViewManager() override;

  /**
   * Returns the pqPVApplicationCore instance. If no pqPVApplicationCore has been
   * created then return nullptr.
   */
  static lqLidarViewManager* instance()
  {
    return qobject_cast<lqLidarViewManager*>(Superclass::instance());
  }

  // LidarView specific
  void pythonStartup() override;
};

#endif // LQLIDARVIEWMANAGER_H
