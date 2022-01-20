#include "oneapi/tbb/enumerable_thread_specific.h"

#if _WIN32 || _WIN64
#include <windows.h>
#else
#include <pthread.h>
#endif

namespace tbb {
namespace detail {
namespace d1 {

#if _WIN32||_WIN64
#if __TBB_WIN8UI_SUPPORT
void __TBB_EXPORTED_FUNC ets_base<ets_key_per_instance>::create_key() {
    my_key = ::FlsAlloc();
}

void __TBB_EXPORTED_FUNC ets_base<ets_key_per_instance>::destroy_key() {
    my_key = ::FlsFree(my_key);
}

void __TBB_EXPORTED_FUNC ets_base<ets_key_per_instance>::set_tls(void * value) {
    ::FlsSetValue(my_key, (LPVOID)value); 
}

void* __TBB_EXPORTED_FUNC ets_base<ets_key_per_instance>::get_tls() {
    return (void *)FlsGetValue(my_key); 
}

#else 
void __TBB_EXPORTED_FUNC ets_base<ets_key_per_instance>::create_key() {
    my_key = ::TlsAlloc();
}

void __TBB_EXPORTED_FUNC ets_base<ets_key_per_instance>::destroy_key() {
    my_key = ::TlsFree(my_key);
}

void __TBB_EXPORTED_FUNC ets_base<ets_key_per_instance>::set_tls(void * value) {
    ::TlsSetValue(my_key, (LPVOID)value); 
}

void* __TBB_EXPORTED_FUNC ets_base<ets_key_per_instance>::get_tls() {
    return (void *)TlsGetValue(my_key); 
}
#endif
#endif

} // namespace d1
} // namespace detail
} // namespace tbb
