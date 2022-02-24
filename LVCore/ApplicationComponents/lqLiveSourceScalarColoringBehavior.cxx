/*=========================================================================

   Program: LidarView
   Module:  lqLiveSourceScalarColoringBehavior.cxx

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
#include "lqLiveSourceScalarColoringBehavior.h"

#include <vtkDataObject.h>
#include <vtkNew.h>

#include <vtkDataSetAttributes.h>
#include <vtkSMSourceProxy.h>
#include <vtkSMViewProxy.h>
#include <vtkSMPVRepresentationProxy.h>
#include <vtkSMParaViewPipelineControllerWithRendering.h>
#include <vtkPVArrayInformation.h>
#include <vtkPVDataInformation.h>
#include <vtkPVDataSetAttributesInformation.h>
#include <vtkPVXMLElement.h>

#include <pqApplicationCore.h>
#include <pqPipelineSource.h>
#include <pqServerManagerModel.h>
#include <pqView.h>


//-----------------------------------------------------------------------------
lqLiveSourceScalarColoringBehavior::lqLiveSourceScalarColoringBehavior(QObject *parent)
  : Superclass(parent)
{
  //Monitor added sources
  pqServerManagerModel* smmodel = pqApplicationCore::instance()->getServerManagerModel();
  this->connect(smmodel, SIGNAL(sourceAdded(pqPipelineSource*)), SLOT(onSourceAdded(pqPipelineSource*)));
}

//-----------------------------------------------------------------------------
void lqLiveSourceScalarColoringBehavior::onSourceAdded(pqPipelineSource* src)
{
  vtkSMProxy* proxy = src->getProxy();
  if (auto hints = proxy->GetHints())
  {
    if (hints->FindNestedElementByName("LiveSource"))
    {
      //Ask live source to report its updates
      this->connect(src, SIGNAL(dataUpdated(pqPipelineSource*)), SLOT(onDataUpdated(pqPipelineSource*)));
    }
  }
}

//-----------------------------------------------------------------------------
void lqLiveSourceScalarColoringBehavior::onDataUpdated(pqPipelineSource* src)
{
  // disconnect dataUpdate Signal, if procedure comes to an end.
  if (this->TrySetScalarColoring(src))
  {
    this->disconnect(src, SIGNAL(dataUpdated(pqPipelineSource*)), this,  SLOT(onDataUpdated(pqPipelineSource*)));
  }
}

//-----------------------------------------------------------------------------
bool lqLiveSourceScalarColoringBehavior::TrySetScalarColoring(pqPipelineSource* src)
{
    // check if at least one representation from an output port is using scalar coloring
    bool hasSucceed = false;

    vtkNew<vtkSMParaViewPipelineController> ppc;
    auto* ppcwr = vtkSMParaViewPipelineControllerWithRendering::SafeDownCast(ppc);
    if ( !ppcwr )
    {
      std::cerr << __FUNCTION__ << " Pipeline Controller has no rendering capabilities" << std::endl;
      return true; // Error, Do not try again
    }

    pqServerManagerModel* smmodel = pqApplicationCore::instance()->getServerManagerModel();
    auto views = smmodel->findItems<pqView*>();
    vtkSMSourceProxy* proxy = src->getSourceProxy();
    int nbPort = proxy->GetNumberOfOutputPorts();

    // for each view
    foreach (pqView* view, views)
    {
      auto* viewProxy = view->getViewProxy();
      // on each port
      for (int i = 0; i < nbPort; ++i)
      {
        auto* pvrp = vtkSMPVRepresentationProxy::SafeDownCast(viewProxy->FindRepresentation(proxy, i));
        // if a representation exist
        if (pvrp)
        {
          // check if the data has a scalar array
          if (proxy->GetDataInformation(i) &&
              proxy->GetDataInformation(i)->GetPointDataInformation() &&
              proxy->GetDataInformation(i)->GetPointDataInformation()->GetAttributeInformation(vtkDataSetAttributes::SCALARS))
          {
            auto* pdInfo = proxy->GetDataInformation(i)->GetPointDataInformation();
            const char* arrayName = pdInfo->GetAttributeInformation(vtkDataSetAttributes::SCALARS)->GetName();
            // check if ScalarColoring is already properly set
            if( !pvrp->GetUsingScalarColoring() )
            {
              pvrp->SetScalarColoring(arrayName, vtkDataObject::AttributeTypes::POINT);
            }
            else
            {
              if (pdInfo->GetArrayInformation(arrayName))
              {
                double range[2];
                pdInfo->GetArrayInformation(arrayName)->GetComponentRange(0, range);
                // check if the given scalar range is not [0,0]
                if (range[0] != 0 || range[1] != 0)
                {
                  pvrp->RescaleTransferFunctionToDataRange(false, false); // Recale transfertFunction only if it's not loocked
                  hasSucceed = true;
                }
              }

            }
          }
        }
      }
    }
  return hasSucceed;
}
