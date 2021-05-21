/*=========================================================================

   Program: LidarView
   Module:  lqLiveSourceScalarColoringBehavior.h

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
#ifndef LQLIVESOURCESCALARCOLORINGBEHAVIOR_H
#define LQLIVESOURCESCALARCOLORINGBEHAVIOR_H

#include <QObject>

#include "lqapplicationcomponents_export.h"

class pqPipelineSource;
class vtkSMSourceProxy;

/**
 * @class lqLiveSourceScalarColoringBehavior
 * @ingroup Behaviors
 *
 * lqLiveSourceScalarColoringBehavior helps to set ScalarColoring,
 * once a LiveSource has received data with an active scalar for the first time.
 * Without it the representation don't set color informations properly
 */
class LQAPPLICATIONCOMPONENTS_EXPORT lqLiveSourceScalarColoringBehavior : public QObject
{
  Q_OBJECT
  typedef QObject Superclass;

public:
  lqLiveSourceScalarColoringBehavior(QObject* parent = 0);

protected slots:
  /**
   * @brief sourceAdded check if the source is a live source and connect
   */
  void onSourceAdded(pqPipelineSource* src);

  virtual void onDataUpdated(pqPipelineSource* src);

protected:
  /**
   * @brief Try to set and rescale the scalar coloring for each representation
   * Return true, if procedure has completed and false it the procedure should
   * be tried again later.
  */
  virtual bool TrySetScalarColoring(pqPipelineSource* src);

private:

  Q_DISABLE_COPY(lqLiveSourceScalarColoringBehavior)
};

#endif // LQLIVESOURCESCALARCOLORINGBEHAVIOR_H
