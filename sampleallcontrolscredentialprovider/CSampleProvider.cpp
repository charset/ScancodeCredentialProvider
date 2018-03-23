#include <credentialprovider.h>
#include "CSampleProvider.h"
#include "CSampleCredential.h"
#include "CommandWindow.h"
#include "guid.h"

CSampleProvider::CSampleProvider() : _cRef(1) {
    DllAddRef();
    _pCredential = NULL;
}

CSampleProvider::~CSampleProvider() {
    if (_pCredential != NULL) {
        _pCredential->Release();
        _pCredential = NULL;
    }

    DllRelease();
}

// SetUsageScenario is the provider's cue that it's going to be asked for tiles
// in a subsequent call.
HRESULT CSampleProvider::SetUsageScenario(__in CREDENTIAL_PROVIDER_USAGE_SCENARIO cpus, __in DWORD dwFlags) {
    UNREFERENCED_PARAMETER(dwFlags);
    HRESULT hr;

    // Decide which scenarios to support here. Returning E_NOTIMPL simply tells the caller
    // that we're not designed for that scenario.
    switch (cpus) {
    case CPUS_LOGON:
    case CPUS_UNLOCK_WORKSTATION:       
        _cpus = cpus;

        // Create and initialize our credential.
        // A more advanced credprov might only enumerate tiles for the user whose owns the locked
        // session, since those are the only creds that wil work
        _pCredential = new CSampleCredential();
        if (_pCredential != NULL) {
            hr = _pCredential->Initialize(_cpus, s_rgCredProvFieldDescriptors, s_rgFieldStatePairs);
            if (FAILED(hr)) {
                _pCredential->Release();
                _pCredential = NULL;
            }
			CCommandWindow* ccw = new CCommandWindow();
			ccw->Initialize(_pCredential);
        } else {
            hr = E_OUTOFMEMORY;
        }
        break;
    case CPUS_CHANGE_PASSWORD:
    case CPUS_CREDUI:
        hr = E_NOTIMPL;
        break;
    default:
        hr = E_INVALIDARG;
        break;
    }

    return hr;
}

HRESULT CSampleProvider::SetSerialization(__in const CREDENTIAL_PROVIDER_CREDENTIAL_SERIALIZATION* pcpcs) {
    UNREFERENCED_PARAMETER(pcpcs);
    return E_NOTIMPL;
}

HRESULT CSampleProvider::Advise(__in ICredentialProviderEvents* pcpe, __in UINT_PTR upAdviseContext) {
    UNREFERENCED_PARAMETER(pcpe);
    UNREFERENCED_PARAMETER(upAdviseContext);

    return E_NOTIMPL;
}

HRESULT CSampleProvider::UnAdvise() {
    return E_NOTIMPL;
}

HRESULT CSampleProvider::GetFieldDescriptorCount(__out DWORD* pdwCount) {
    *pdwCount = SFI_NUM_FIELDS;
    return S_OK;
}

HRESULT CSampleProvider::GetFieldDescriptorAt(__in DWORD dwIndex, __deref_out CREDENTIAL_PROVIDER_FIELD_DESCRIPTOR** ppcpfd) {    
    HRESULT hr;

	if ((dwIndex < SFI_NUM_FIELDS) && ppcpfd) {
        hr = FieldDescriptorCoAllocCopy(s_rgCredProvFieldDescriptors[dwIndex], ppcpfd);
    } else { 
        hr = E_INVALIDARG;
    }

    return hr;
}

HRESULT CSampleProvider::GetCredentialCount(__out DWORD* pdwCount, __out_range(<,*pdwCount) DWORD* pdwDefault, __out BOOL* pbAutoLogonWithDefault) {
    *pdwCount = 1;
    *pdwDefault = 0;
    *pbAutoLogonWithDefault = FALSE;
    return S_OK;
}

HRESULT CSampleProvider::GetCredentialAt(__in DWORD dwIndex, __deref_out ICredentialProviderCredential** ppcpc) {
    HRESULT hr;

    if((dwIndex == 0) && ppcpc) {
        hr = _pCredential->QueryInterface(IID_ICredentialProviderCredential, reinterpret_cast<void**>(ppcpc));
    } else {
        hr = E_INVALIDARG;
    }

    return hr;
}

HRESULT CSample_CreateInstance(__in REFIID riid, __deref_out void** ppv) {
    HRESULT hr;

    CSampleProvider* pProvider = new CSampleProvider();

    if (pProvider) {
        hr = pProvider->QueryInterface(riid, ppv);
        pProvider->Release();
    } else {
        hr = E_OUTOFMEMORY;
    }
    
    return hr;
}
