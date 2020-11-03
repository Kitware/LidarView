#include "lqHelper.h"

#include "vtkLidarReader.h"
#include "vtkLidarStream.h"

//-----------------------------------------------------------------------------
bool IsLidarProxy(vtkSMProxy * proxy)
{
  if(proxy != nullptr)
  {
    auto* tmp_stream = dynamic_cast<vtkLidarStream*> (proxy->GetClientSideObject());
    auto* tmp_reader = dynamic_cast<vtkLidarReader*> (proxy->GetClientSideObject());
    if (tmp_stream || tmp_reader)
    {
      return true;
    }
  }
  return false;
}
