//
// THIS CODE AND INFORMATION IS PROVIDED "AS IS" WITHOUT WARRANTY OF
// ANY KIND, EITHER EXPRESSED OR IMPLIED, INCLUDING BUT NOT LIMITED TO
// THE IMPLIED WARRANTIES OF MERCHANTABILITY AND/OR FITNESS FOR A
// PARTICULAR PURPOSE.
//
// Copyright (c) Microsoft Corporation. All rights reserved.
//

#include <windows.h>      // For common windows data types and function headers
//#define STRICT_TYPED_ITEMIDS
#include <shlobj.h>
//#include <shtypes.h>      // for COMDLG_FILTERSPEC

#pragma comment(linker, "\"/manifestdependency:type='Win32' name='Microsoft.Windows.Common-Controls' version='6.0.0.0' processorArchitecture='*' publicKeyToken='6595b64144ccf1df' language='*'\"")

#include <filesystem>
#include <util/utility.h>
#include "filedialog.h"

const COMDLG_FILTERSPEC c_rgSaveTypes[] =
{
    {L"Word Document (*.doc)",       L"*.doc"},
    {L"Web Page (*.htm; *.html)",    L"*.htm;*.html"},
    {L"Text Document (*.txt)",       L"*.txt"},
    {L"All Documents (*.*)",         L"*.*"}
};

// Indices of file types
#define INDEX_WORDDOC 1
#define INDEX_WEBPAGE 2
#define INDEX_TEXTDOC 3

/* Common File Dialog Snippets ***************************************************************************************************/

// This code snippet demonstrates how to work with the common file dialog interface
std::filesystem::path GetFolderOpen()
{
    // CoCreate the File Open Dialog object.
    using std::filesystem::path;
    path defaultPath;

    IFileDialog* pfd = NULL;
    HRESULT hr = CoCreateInstance(CLSID_FileOpenDialog, NULL, CLSCTX_INPROC_SERVER, IID_PPV_ARGS(&pfd));
    if (!SUCCEEDED(hr))
        return defaultPath;

	DEFER(pfd->Release());

	// Set the options on the dialog.
	DWORD dwFlags;

	// Before setting, always get the options first in order not to override existing options.
	hr = pfd->GetOptions(&dwFlags);
    if (!SUCCEEDED(hr))
        return defaultPath;

	// In this case, get shell items only for file system items.
    
	hr = pfd->SetOptions(dwFlags | FOS_FORCEFILESYSTEM | FOS_PICKFOLDERS);
    if (!SUCCEEDED(hr))
        return defaultPath;

	// Show the dialog
	hr = pfd->Show(NULL);
    if (!SUCCEEDED(hr))
        return defaultPath;

	// Obtain the result, once the user clicks the 'Open' button.
	// The result is an IShellItem object.
	IShellItem* psiResult;
	hr = pfd->GetResult(&psiResult);
    if (!SUCCEEDED(hr))
        return defaultPath;

	// We are just going to print out the name of the file for sample sake.
	PWSTR pszFilePath = NULL;

	hr = psiResult->GetDisplayName(SIGDN_FILESYSPATH, &pszFilePath);
    if (!SUCCEEDED(hr))
        return defaultPath;

    DEFER(psiResult->Release());
	DEFER(CoTaskMemFree(pszFilePath));

    path resultPath = pszFilePath;

    return resultPath;
}
