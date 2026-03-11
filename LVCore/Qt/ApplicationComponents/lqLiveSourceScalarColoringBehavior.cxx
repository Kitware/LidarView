/*=========================================================================

  Program: LidarView
  Module:  lqLiveSourceScalarColoringBehavior.cxx

  Copyright (c) Kitware Inc.
  All rights reserved.
  See LICENSE or http://www.apache.org/licenses/LICENSE-2.0 for details.

  This software is distributed WITHOUT ANY WARRANTY; without even
  the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
  PURPOSE.  See the above copyright notice for more information.

=========================================================================*/

#include "lqLiveSourceScalarColoringBehavior.h"

#include <vtkDataObject.h>
#include <vtkLogger.h>
#include <vtkNew.h>

#include <vtkDataSetAttributes.h>
#include <vtkPVArrayInformation.h>
#include <vtkPVDataInformation.h>
#include <vtkPVDataSetAttributesInformation.h>
#include <vtkPVXMLElement.h>
#include <vtkSMPVRepresentationProxy.h>
#include <vtkSMParaViewPipelineControllerWithRendering.h>
#include <vtkSMSourceProxy.h>
#include <vtkSMViewProxy.h>

#include <pqApplicationCore.h>
#include <pqPipelineSource.h>
#include <pqServerManagerModel.h>
#include <pqView.h>

//-----------------------------------------------------------------------------
lqLiveSourceScalarColoringBehavior::lqLiveSourceScalarColoringBehavior(QObject* parent)
  : Superclass(parent)
{
  // Monitor added sources
  pqServerManagerModel* smmodel = pqApplicationCore::instance()->getServerManagerModel();
  this->connect(
    smmodel, SIGNAL(sourceAdded(pqPipelineSource*)), SLOT(onSourceAdded(pqPipelineSource*)));
}

//-----------------------------------------------------------------------------
void lqLiveSourceScalarColoringBehavior::onSourceAdded(pqPipelineSource* src)
{
  vtkSMProxy* proxy = src->getProxy();
  if (auto hints = proxy->GetHints())
  {
    if (hints->FindNestedElementByName("LiveSource"))
    {
      // Ask live source to report its updates
      this->connect(
        src, SIGNAL(dataUpdated(pqPipelineSource*)), SLOT(onDataUpdated(pqPipelineSource*)));
    }
  }
}

//-----------------------------------------------------------------------------
void lqLiveSourceScalarColoringBehavior::onDataUpdated(pqPipelineSource* src)
{
  // disconnect dataUpdate Signal, if procedure comes to an end.
  if (this->TrySetScalarColoring(src))
  {
    this->disconnect(
      src, SIGNAL(dataUpdated(pqPipelineSource*)), this, SLOT(onDataUpdated(pqPipelineSource*)));
  }
}

//-----------------------------------------------------------------------------
bool lqLiveSourceScalarColoringBehavior::TrySetScalarColoring(pqPipelineSource* src)
{
  // check if at least one representation from an output port is using scalar coloring
  bool hasSucceed = false;

  vtkNew<vtkSMParaViewPipelineController> ppc;
  auto* ppcwr = vtkSMParaViewPipelineControllerWithRendering::SafeDownCast(ppc);
  if (!ppcwr)
  {
    vtkLog(WARNING, << __FUNCTION__ << " Pipeline Controller has no rendering capabilities");
    return true; // Error, Do not try again
  }

  pqServerManagerModel* smmodel = pqApplicationCore::instance()->getServerManagerModel();
  auto views = smmodel->findItems<pqView*>();
  vtkSMSourceProxy* proxy = src->getSourceProxy();
  int nbPort = proxy->GetNumberOfOutputPorts();

  // for each view
  Q_FOREACH (pqView* view, views)
  {
    auto* viewProxy = view->getViewProxy();
    // on each port
    for (int i = 0; i < nbPort; ++i)
    {
      auto* pvrp =
        vtkSMPVRepresentationProxy::SafeDownCast(viewProxy->FindRepresentation(proxy, i));
      // if a representation exist
      if (pvrp)
      {
        // check if the data has a scalar array
        if (proxy->GetDataInformation(i) &&
          proxy->GetDataInformation(i)->GetPointDataInformation() &&
          proxy->GetDataInformation(i)->GetPointDataInformation()->GetAttributeInformation(
            vtkDataSetAttributes::SCALARS))
        {
          auto* pdInfo = proxy->GetDataInformation(i)->GetPointDataInformation();
          const char* arrayName =
            pdInfo->GetAttributeInformation(vtkDataSetAttributes::SCALARS)->GetName();
          // check if ScalarColoring is already properly set
          if (!pvrp->GetUsingScalarColoring())
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
                pvrp->RescaleTransferFunctionToDataRange(
                  false, false); // Recale transfertFunction only if it's not loocked
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
