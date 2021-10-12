/*=========================================================================

   Program: LidarView
   Module:  lqLidarViewManager.cxx

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
#include "lqLidarViewManager.h"

#include "lqPythonQtLidarView.h"
#include "lqOpenPcapReaction.h"

#include <QFileInfo>

//-----------------------------------------------------------------------------
lqLidarViewManager::lqLidarViewManager(QObject* parent /*=nullptr*/)
  : Superclass(parent)
{

}

//-----------------------------------------------------------------------------
lqLidarViewManager::~lqLidarViewManager()
{

}
//-----------------------------------------------------------------------------
void lqLidarViewManager::pythonStartup()
{
  // Register LidarView specific decorators first
  PythonQt::self()->addDecorators(new lqPythonQtLidarView(this));

  Superclass::pythonStartup();
}


//-----------------------------------------------------------------------------
void lqLidarViewManager::openData(const QString& filename)
{
  if (QFileInfo(filename).suffix() == "pcap")
  {
    lqOpenPcapReaction::createSourceFromFile(filename);
  }
  else
  {
    this->runPython(QString("lv.openData('%1')\n").arg(filename));
  }
}

