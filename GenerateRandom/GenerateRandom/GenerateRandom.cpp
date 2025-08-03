// GenerateRandom.cpp : Defines the entry point for the application.
//

#include "framework.h"
#include <shlwapi.h>
#include <shobjidl_core.h>
#include <wincrypt.h>
#include <windowsx.h>
#include "GenerateRandom.h"

#pragma comment(lib, "imm32.lib")
#pragma comment(lib, "shlwapi.lib")

INT_PTR CALLBACK    MainBox(HWND, UINT, WPARAM, LPARAM);
INT_PTR CALLBACK    UpperProc(HWND, UINT, WPARAM, LPARAM);
INT_PTR CALLBACK    LowerProc(HWND, UINT, WPARAM, LPARAM);
INT_PTR CALLBACK    DigitProc(HWND, UINT, WPARAM, LPARAM);
INT_PTR CALLBACK    OtherProc(HWND, UINT, WPARAM, LPARAM);
LPSTR               GenRndStr(int, int, LPSTR, LPSTR *);
LPBYTE              GenRndData(int);
BOOL                GetOutFilePath(HWND, LPWSTR *);
void                SetMode(HWND, BOOL);

int lenBuf;
LPBYTE pBuf;
HANDLE hHeap;
HCRYPTPROV hProv;
WNDPROC edtUpper, edtLower, edtDigit, edtOther;

int APIENTRY wWinMain(_In_ HINSTANCE hInstance,
                     _In_opt_ HINSTANCE hPrevInstance,
                     _In_ LPWSTR    lpCmdLine,
                     _In_ int       nCmdShow)
{
    hHeap = GetProcessHeap();
    DialogBox(hInstance, MAKEINTRESOURCE(IDD_MAINBOX), NULL, MainBox);
    return 0;
}

// Message handler for main box.
INT_PTR CALLBACK MainBox(HWND hDlg, UINT message, WPARAM wParam, LPARAM lParam)
{
    switch (message)
    {
    case WM_INITDIALOG:
        CryptAcquireContextA(&hProv, NULL, NULL, PROV_RSA_FULL, CRYPT_VERIFYCONTEXT);
        Button_SetCheck(GetDlgItem(hDlg, IDC_RBTNSTR), BST_CHECKED);
        Button_SetCheck(GetDlgItem(hDlg, IDC_RBTNBYTE), BST_CHECKED);
        SetWindowTextA(GetDlgItem(hDlg, IDC_EDTUPPER), "ABCDEFGHIJKLMNOPQRSTUVWXYZ");
        //ImmAssociateContext(hCtrl, NULL);
        SetWindowTextA(GetDlgItem(hDlg, IDC_EDTLOWER), "abcdefghijklmnopqrstuvwxyz");
        //ImmAssociateContext(hCtrl, NULL);
        SetWindowTextA(GetDlgItem(hDlg, IDC_EDTDIGIT), "0123456789");
        //ImmAssociateContext(hCtrl, NULL);
        SetWindowTextA(GetDlgItem(hDlg, IDC_EDTOTHER), "!\"#$%&'()*+,-./:;<=>?@[\\]^_`{|}~");
        //ImmAssociateContext(hCtrl, NULL);
        edtUpper = (WNDPROC)SetWindowLongPtrA(GetDlgItem(hDlg, IDC_EDTUPPER), GWLP_WNDPROC, (LONG_PTR)UpperProc);
        edtLower = (WNDPROC)SetWindowLongPtrA(GetDlgItem(hDlg, IDC_EDTLOWER), GWLP_WNDPROC, (LONG_PTR)LowerProc);
        edtDigit = (WNDPROC)SetWindowLongPtrA(GetDlgItem(hDlg, IDC_EDTDIGIT), GWLP_WNDPROC, (LONG_PTR)DigitProc);
        edtOther = (WNDPROC)SetWindowLongPtrA(GetDlgItem(hDlg, IDC_EDTOTHER), GWLP_WNDPROC, (LONG_PTR)OtherProc);
        return (INT_PTR)TRUE;

    case WM_COMMAND:
        {
            int wmId = LOWORD(wParam);
            switch (wmId)
            {
            case IDC_RBTNSTR:
                SetMode(hDlg, TRUE);
                break;
            case IDC_RBTNDATA:
                SetMode(hDlg, FALSE);
                break;
            case IDC_CHKUPPER:
            case IDC_CHKLOWER:
            case IDC_CHKDIGIT:
            case IDC_CHKOTHER:
                if (Button_GetCheck(GetDlgItem(hDlg, wmId)) == BST_CHECKED)
                    Edit_SetReadOnly(GetDlgItem(hDlg, wmId + 1), FALSE);
                else
                    Edit_SetReadOnly(GetDlgItem(hDlg, wmId + 1), TRUE);
                break;
            case IDC_BTNEXPORT:
                if (lenBuf)
                {
                    LPWSTR pwszPath2Save = NULL;
                    if (GetOutFilePath(hDlg, &pwszPath2Save))
                    {
                        HANDLE hFile = CreateFileW(pwszPath2Save, GENERIC_WRITE, NULL, NULL, CREATE_ALWAYS, FILE_ATTRIBUTE_NORMAL, NULL);
                        if (hFile != INVALID_HANDLE_VALUE)
                        {
                            DWORD dwSize = (DWORD)lenBuf;
                            WriteFile(hFile, pBuf, dwSize, &dwSize, NULL);
                            CloseHandle(hFile);
                            MessageBoxA(hDlg, "Successfully Exported Random Data!", NULL, MB_OK);
                        }
                        else
                            MessageBoxA(hDlg, "Failed Exporting Random Data!", NULL, MB_OK);
                        HeapFree(hHeap, 0, pwszPath2Save);
                    }
                    else
                        MessageBoxA(hDlg, "Exportion of Random Data Cancelled!", NULL, MB_OK);
                }
                else
                    MessageBoxA(hDlg, "Random Data Should be Generated Before Exported!", NULL, MB_OK);
                break;
            case IDC_BTNGENERATE:
                if (Button_GetCheck(GetDlgItem(hDlg, IDC_RBTNSTR)) == BST_CHECKED)
                {
                    HWND hCtrl = GetDlgItem(hDlg, IDC_EDTLENSTR);
                    int numTypes = 0, lenStr = GetWindowTextLengthW(hCtrl);
                    if (lenStr > 0)
                    {
                        LPSTR pszLen = (LPSTR)HeapAlloc(hHeap, HEAP_ZERO_MEMORY, lenStr + 1);
                        GetWindowTextA(hCtrl, pszLen, lenStr + 1);
                        lenStr = StrToIntA(pszLen);
                        HeapFree(hHeap, 0, pszLen);
                    }
                    if ((lenStr >= 8) && (lenStr <= 256))
                    {
                        LPSTR ppszSeeds[4] = { NULL,NULL,NULL,NULL };
                        CHAR pszTypes[5] = { 0,0,0,0,0 };
                        int maxLen[4] = { 27,27,11,33 };
                        for (int i = 0; i < 4; i ++)
                        {
                            if (Button_GetCheck(GetDlgItem(hDlg, IDC_CHKUPPER + 2 * i)) == BST_CHECKED)
                            {
                                hCtrl = GetDlgItem(hDlg, IDC_EDTUPPER + 2 * i);
                                ppszSeeds[i] = (LPSTR)HeapAlloc(hHeap, HEAP_ZERO_MEMORY, maxLen[i]);
                                GetWindowTextA(hCtrl, ppszSeeds[i], maxLen[i]);
                                pszTypes[numTypes] = i + 0x30;
                                numTypes += 1;
                            }
                        }
                        if (numTypes)
                        {
                            LPSTR pszOut = GenRndStr(lenStr, numTypes, pszTypes, ppszSeeds);
                            SetWindowTextA(GetDlgItem(hDlg, IDC_EDTSTROUT), pszOut);
                            HeapFree(hHeap, 0, pszOut);
                        }
                        else
                            MessageBoxA(hDlg, "Should Select AT LEAST ONE Type of Character...", NULL, MB_OK);
                        for (int i = 0; i < 4; i++)
                            if (ppszSeeds[i])
                                HeapFree(hHeap, 0, ppszSeeds[i]);
                    }
                    else                     
                        MessageBoxA(hDlg, "Random String Length Should be Set Between 8 and 256...", NULL, MB_OK);
                }
                else
                {
                    HWND hCtrl = GetDlgItem(hDlg, IDC_EDTLENDATA);
                    int sizeElement = 1, lenData = GetWindowTextLengthA(hCtrl);
                    if (lenData > 0)
                    {
                        LPSTR pszLen = (LPSTR)HeapAlloc(hHeap, HEAP_ZERO_MEMORY, lenData + 1);
                        GetWindowTextA(hCtrl, pszLen, lenData + 1);
                        lenData = StrToIntA(pszLen);
                        HeapFree(hHeap, 0, pszLen);
                    }
                    if ((lenData >= 8) && (lenData <= 65536))
                    {
                        for (int i = 0; i < 3; i++)
                        {
                            if (Button_GetCheck(GetDlgItem(hDlg, IDC_RBTNBYTE + i)) == BST_CHECKED)
                                break;
                            sizeElement <<= 1;
                        }
                        lenBuf = sizeElement * lenData;
                        LPBYTE pDataBuf = GenRndData(lenBuf);
                        if (pBuf)
                            HeapFree(hHeap, 0, pBuf);
                        pBuf = pDataBuf;
                        LPSTREAM pStrm = SHCreateMemStream(NULL, 0);
                        CHAR pszTmp[MAX_PATH];
                        IStream_Write(pStrm, "{", 1);
                        for (int i = 0; i < lenData; i++)
                        {
                            if (i)
                                IStream_Write(pStrm, ",", 1);
                            if((i*sizeElement) %16==0)
                                IStream_Write(pStrm, "\r\n\t", 3);
                            else
                                IStream_Write(pStrm, " ", 1);
                            IStream_Write(pStrm, "0x", 2);
                            switch (sizeElement)
                            {
                            case 1:
                                wsprintfA(pszTmp, "%02X", *pDataBuf);
                                break;
                            case 2:
                                wsprintfA(pszTmp, "%04X", *((LPWORD)pDataBuf));
                                break;
                            case 4:
                                wsprintfA(pszTmp, "%08X", *((LPDWORD)pDataBuf));
                                break;
                            case 8:
                                wsprintfA(pszTmp, "%016IXULL", *((UINT64 *)pDataBuf));
                                break;
                            }
                            IStream_Write(pStrm, pszTmp, lstrlenA(pszTmp));
                            pDataBuf += sizeElement;
                        }
                        IStream_Write(pStrm, "\r\n}", 3);
                        ULARGE_INTEGER ulSize;
                        IStream_Size(pStrm, &ulSize);
                        IStream_Reset(pStrm);
                        LPSTR pszStrC = (LPSTR)HeapAlloc(hHeap, HEAP_ZERO_MEMORY, ulSize.LowPart + 1);
                        IStream_Read(pStrm, pszStrC, ulSize.LowPart);
                        IUnknown_AtomicRelease((LPVOID*)&pStrm);
                        SetWindowTextA(GetDlgItem(hDlg, IDC_EDTDATAOUT), pszStrC);
                        HeapFree(hHeap, 0, pszStrC);
                    }
                    else
                        MessageBoxA(hDlg, "Number of Random Data Elements Should be Set Between 8 and 65536!", NULL, MB_OK);
                }
                break;
            case IDCANCEL:
                DestroyWindow(hDlg);
                break;
            }
        }
        break;
    case WM_DESTROY:
        if (pBuf)
            HeapFree(hHeap, 0, pBuf);
        CryptReleaseContext(hProv, 0);
        PostQuitMessage(0);
        break;
    }
    return (INT_PTR)FALSE;
}

LRESULT CALLBACK UpperProc(HWND hEdit, UINT message, WPARAM wParam, LPARAM lParam)
{
    CHAR pszText[27], keyId;
    int lenStr;
    switch (message)
    {        
    case WM_CHAR:
        keyId = (CHAR)wParam;
        GetWindowTextA(hEdit, pszText, 27);
        lenStr = lstrlenA(pszText);
        if ((keyId < 0x20) || (keyId > 0x7e))
            break;
        if (lenStr >= 26)
            return 0;
        if(IsCharUpperA(keyId))
        {
            if (StrChrA(pszText, keyId))
                return 0;
        }
        else
            return 0;
        break;

    case WM_DESTROY:
        SetWindowLongPtrA(hEdit, GWLP_WNDPROC, (LONG_PTR)edtUpper);
        return 0;
    }

    return CallWindowProc(edtUpper, hEdit, message, wParam, lParam);
}

LRESULT CALLBACK LowerProc(HWND hEdit, UINT message, WPARAM wParam, LPARAM lParam)
{
    CHAR pszText[27], keyId;
    int lenStr;
    switch (message)
    {
    case WM_CHAR:
        keyId = (CHAR)wParam;
        GetWindowTextA(hEdit, pszText, 27);
        lenStr = lstrlenA(pszText);
        if ((keyId < 0x20) || (keyId > 0x7e))
            break;
        if (lenStr >= 26)
            return 0;
        if (IsCharLowerA(keyId))
        {
            if (StrChrA(pszText, keyId))
                return 0;
        }
        else
            return 0;
        break;

    case WM_DESTROY:
        SetWindowLongPtrA(hEdit, GWLP_WNDPROC, (LONG_PTR)edtLower);
        return 0;
    }

    return CallWindowProc(edtLower, hEdit, message, wParam, lParam);
}

LRESULT CALLBACK DigitProc(HWND hEdit, UINT message, WPARAM wParam, LPARAM lParam)
{
    CHAR pszText[11], keyId;
    int lenStr;
    switch (message)
    {
    case WM_CHAR:
        keyId = (CHAR)wParam;
        GetWindowTextA(hEdit, pszText, 11);
        lenStr = lstrlenA(pszText);
        if ((keyId < 0x20) || (keyId > 0x7e))
            break;
        if (lenStr >= 10)
            return 0;
        if (IsCharAlphaNumericA(keyId) && !IsCharAlphaA(keyId))
        {
            if (StrChrA(pszText, keyId))
                return 0;
        }
        else
            return 0;
        break;

    case WM_DESTROY:
        SetWindowLongPtrA(hEdit, GWLP_WNDPROC, (LONG_PTR)edtDigit);
        return 0;
    }

    return CallWindowProc(edtDigit, hEdit, message, wParam, lParam);
}

LRESULT CALLBACK OtherProc(HWND hEdit, UINT message, WPARAM wParam, LPARAM lParam)
{
    CHAR pszText[33], keyId;
    int lenStr;
    switch (message)
    {
    case WM_CHAR:
        keyId = (CHAR)wParam;
        GetWindowTextA(hEdit, pszText, 33);
        lenStr = lstrlenA(pszText);
        if ((keyId < 0x20) || (keyId > 0x7e))
            break;
        if (lenStr >= 32)
            return 0;
        if (!IsCharAlphaNumericA(keyId) && keyId != 0x20)
        {
            if (StrChrA(pszText, keyId))
                return 0;
        }
        else
            return 0;
        break;

    case WM_DESTROY:
        SetWindowLongPtrA(hEdit, GWLP_WNDPROC, (LONG_PTR)edtOther);
        return 0;
    }

    return CallWindowProc(edtOther, hEdit, message, wParam, lParam);
}

LPSTR GenRndStr(int lenStr, int numTypes, LPSTR pszTypes, LPSTR *ppszSeed)
{
    LPSTR pszOut = (LPSTR)HeapAlloc(hHeap, HEAP_ZERO_MEMORY, lenStr + 1), pszTmp = (LPSTR)HeapAlloc(hHeap, HEAP_ZERO_MEMORY, lenStr + 1);
    LPSTR ppszSeeds[4] = { *ppszSeed , *(ppszSeed + 1), *(ppszSeed + 2), *(ppszSeed + 3) };
    int idxTmp, lenSeeds[4] = { 0,0,0,0 };
    for (int i = 0; i < 4; i++)
        if (ppszSeeds[i])
            lenSeeds[i] = lstrlenA(ppszSeeds[i]);
    LPWORD pwBufRand = (LPWORD)HeapAlloc(hHeap, HEAP_ZERO_MEMORY, lenStr * 6);
    LPWORD pwBuf2Calc = pwBufRand;
    CryptGenRandom(hProv, lenStr * 6, (LPBYTE)pwBufRand);
    lstrcpyA(pszTmp, pszTypes);
    for (int i = numTypes; i < lenStr; i++)
    {
        pszTmp[i] = pszTypes[pwBuf2Calc[0] % numTypes];
        pwBuf2Calc += 1;
    }
    for (int i = 0; i < lenStr; i++)
    {
        int lenTmp = lstrlenA(pszTmp);
        idxTmp = pwBuf2Calc[0] % lenTmp;
        lenTmp -= 1;
        pszOut[i] = pszTmp[idxTmp];
        pszTmp[idxTmp] = pszTmp[lenTmp];
        pszTmp[lenTmp] = L'\0';
        pwBuf2Calc += 1;
    }
    for (int i = 0; i < lenStr; i++)
    {
        idxTmp = pszOut[i] - 0x30;
        pszOut[i] = ppszSeeds[idxTmp][pwBuf2Calc[0] % lenSeeds[idxTmp]];
        pwBuf2Calc += 1;
    }
    HeapFree(hHeap, 0, pwBufRand);
    HeapFree(hHeap, 0, pszTmp);
    return pszOut;
}

LPBYTE GenRndData(int lenData)
{
    LPBYTE pBufData = (LPBYTE)HeapAlloc(hHeap, HEAP_ZERO_MEMORY, lenData);
    CryptGenRandom(hProv, lenData, pBufData);
    return pBufData;
}

BOOL GetOutFilePath(HWND hDlg, LPWSTR * pwszPath)
{
    BOOL bRet = FALSE;
    IFileSaveDialog* pFileSave;
    HRESULT hr;
    hr = CoCreateInstance(CLSID_FileSaveDialog, NULL, CLSCTX_ALL, IID_IFileSaveDialog, (LPVOID*)&pFileSave);
    if (SUCCEEDED(hr))
    {
        COMDLG_FILTERSPEC rgSpec[] =
        {
            { L"Raw Binary File", L"*.bin" }
        };
        pFileSave->SetFileTypes(1, rgSpec);
        hr = pFileSave->Show(hDlg);
        if (SUCCEEDED(hr))
        {
            IShellItem* pItem;
            hr = pFileSave->GetResult(&pItem);
            if (SUCCEEDED(hr))
            {
                PWSTR pwszTmpPath;
                hr = pItem->GetDisplayName(SIGDN_FILESYSPATH, &pwszTmpPath);
                if (SUCCEEDED(hr))
                {
                    if (*pwszPath)
                        HeapFree(hHeap, 0, *pwszPath);
                    size_t lenTmp = lstrlenW(pwszTmpPath) + 1;
                    *pwszPath = (LPWSTR)HeapAlloc(hHeap, HEAP_ZERO_MEMORY, lenTmp * 2);
                    lstrcpyW(*pwszPath, pwszTmpPath);
                    CoTaskMemFree(pwszTmpPath);
                    bRet = TRUE;
                }
                pItem->Release();
            }
        }
        pFileSave->Release();
    }
    return bRet;
}

void SetMode(HWND hDlg, BOOL bStrMode)
{
    Edit_SetReadOnly(GetDlgItem(hDlg, IDC_EDTLENSTR), !bStrMode);
    EnableWindow(GetDlgItem(hDlg, IDC_CHKUPPER), bStrMode);
    Edit_SetReadOnly(GetDlgItem(hDlg, IDC_EDTUPPER), !IsDlgButtonChecked(hDlg, IDC_CHKUPPER) || !bStrMode);
    EnableWindow(GetDlgItem(hDlg, IDC_CHKLOWER), bStrMode);
    Edit_SetReadOnly(GetDlgItem(hDlg, IDC_EDTLOWER), !IsDlgButtonChecked(hDlg, IDC_CHKLOWER) || !bStrMode);
    EnableWindow(GetDlgItem(hDlg, IDC_CHKDIGIT), bStrMode);
    Edit_SetReadOnly(GetDlgItem(hDlg, IDC_EDTDIGIT), !IsDlgButtonChecked(hDlg, IDC_CHKDIGIT) || !bStrMode);
    EnableWindow(GetDlgItem(hDlg, IDC_CHKOTHER), bStrMode);
    Edit_SetReadOnly(GetDlgItem(hDlg, IDC_EDTOTHER), !IsDlgButtonChecked(hDlg, IDC_CHKOTHER) || !bStrMode);
    Edit_SetReadOnly(GetDlgItem(hDlg, IDC_EDTLENDATA), bStrMode);
    EnableWindow(GetDlgItem(hDlg, IDC_RBTNBYTE), !bStrMode);
    EnableWindow(GetDlgItem(hDlg, IDC_RBTNWORD), !bStrMode);
    EnableWindow(GetDlgItem(hDlg, IDC_RBTNDWORD), !bStrMode);
    EnableWindow(GetDlgItem(hDlg, IDC_RBTNQWORD), !bStrMode);
    EnableWindow(GetDlgItem(hDlg, IDC_BTNEXPORT), !bStrMode);
}