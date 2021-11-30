#pragma once

#include <p7/P7_Trace.h>
#include <functional>

namespace adst::common {

template <typename TraceObjT>
class ManageTraceObj
{
public:
    typedef int32_t (TraceObjT::*ReleaseMemFnc)();

    ManageTraceObj(TraceObjT* (*creator)(const char*), const char* name, ReleaseMemFnc releaseMemFnc)
    {
        resource_      = creator(name);
        releaseMemFnc_ = releaseMemFnc;
    }

    ManageTraceObj(TraceObjT* (*creator)(IP7_Client*, const char*, const stTrace_Conf*), IP7_Client* client,
                   const char* name, ReleaseMemFnc releaseMemFnc)
    {
        resource_      = creator(client, name, nullptr);
        releaseMemFnc_ = releaseMemFnc;
    }

    TraceObjT* getObj()
    {
        return resource_;
    }

    ~ManageTraceObj()
    {
        if (resource_ && releaseMemFnc_) {
            std::invoke(releaseMemFnc_, *resource_);
        }
    }

private:
    TraceObjT*    resource_      = nullptr;
    ReleaseMemFnc releaseMemFnc_ = nullptr;
};

} // namespace adst::common
