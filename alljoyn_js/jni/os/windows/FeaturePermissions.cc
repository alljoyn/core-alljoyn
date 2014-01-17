/*
 * Copyright (c) 2011-2012, AllSeen Alliance. All rights reserved.
 *
 *    Permission to use, copy, modify, and/or distribute this software for any
 *    purpose with or without fee is hereby granted, provided that the above
 *    copyright notice and this permission notice appear in all copies.
 *
 *    THE SOFTWARE IS PROVIDED "AS IS" AND THE AUTHOR DISCLAIMS ALL WARRANTIES
 *    WITH REGARD TO THIS SOFTWARE INCLUDING ALL IMPLIED WARRANTIES OF
 *    MERCHANTABILITY AND FITNESS. IN NO EVENT SHALL THE AUTHOR BE LIABLE FOR
 *    ANY SPECIAL, DIRECT, INDIRECT, OR CONSEQUENTIAL DAMAGES OR ANY DAMAGES
 *    WHATSOEVER RESULTING FROM LOSS OF USE, DATA OR PROFITS, WHETHER IN AN
 *    ACTION OF CONTRACT, NEGLIGENCE OR OTHER TORTIOUS ACTION, ARISING OUT OF
 *    OR IN CONNECTION WITH THE USE OR PERFORMANCE OF THIS SOFTWARE.
 */
#include "../../FeaturePermissions.h"

#include "../../PluginData.h"
#include "resource.h"
#include <qcc/Debug.h>
#include <qcc/FileStream.h>
#include <qcc/StringUtil.h>
#include <qcc/Util.h>

#define QCC_MODULE "ALLJOYN_JS"

class RequestPermissionContext {
  public:
    qcc::String origin;
    bool remember;
    RequestPermissionContext() : remember(false) { }
};

static LRESULT CALLBACK DlgProc(HWND hwnd, UINT msg, WPARAM wParam, LPARAM lParam)
{
    switch (msg) {
    case WM_INITDIALOG: {
            QCC_DbgTrace(("%s(hwnd=%d,msg=WM_INITDIALOG,wParam=0x%x,lParam=0x%x)", __FUNCTION__, hwnd, wParam, lParam));
            /*
             * Save the context pointer.
             */
            RequestPermissionContext* context = reinterpret_cast<RequestPermissionContext*>(lParam);
            SetWindowLongPtr(hwnd, GWLP_USERDATA, reinterpret_cast<LONG_PTR>(context));
            /*
             * Fill in the origin control with the right value.
             */
            int lenW = MultiByteToWideChar(CP_ACP, 0, context->origin.c_str(), -1, NULL, 0);
            wchar_t* originW = new wchar_t[lenW];
            MultiByteToWideChar(CP_ACP, 0, context->origin.c_str(), -1, originW, lenW);
            if (!SetDlgItemText(hwnd, IDC_ORIGIN, originW)) {
                QCC_LogError(ER_OS_ERROR, ("SetDlgItemText failed - %d", GetLastError()));
            }
            delete[] originW;
            /*
             * Center the dialog window.
             */
            HWND parent = GetParent(hwnd);
            if (NULL == parent) {
                parent = GetDesktopWindow();
            }
            RECT parentRect;
            if (!GetWindowRect(parent, &parentRect)) {
                QCC_LogError(ER_OS_ERROR, ("GetWindowRect(parent) failed - %d", GetLastError()));
                break;
            }
            RECT rect;
            if (!GetWindowRect(hwnd, &rect)) {
                QCC_LogError(ER_OS_ERROR, ("GetWindowRect failed - %d", GetLastError()));
                break;
            }
            RECT offset;
            if (!CopyRect(&offset, &parentRect)) {
                QCC_LogError(ER_OS_ERROR, ("CopyRect failed - %d", GetLastError()));
                break;
            }
            if (!OffsetRect(&rect, -rect.left, -rect.top)) {
                QCC_LogError(ER_OS_ERROR, ("OffsetRect failed - %d", GetLastError()));
                break;
            }
            if (!OffsetRect(&offset, -offset.left, -offset.top)) {
                QCC_LogError(ER_OS_ERROR, ("OffsetRect failed - %d", GetLastError()));
                break;
            }
            if (!OffsetRect(&offset, -rect.right, -rect.bottom)) {
                QCC_LogError(ER_OS_ERROR, ("OffsetRect failed - %d", GetLastError()));
                break;
            }
            if (!SetWindowPos(hwnd, HWND_TOP, parentRect.left + (offset.right / 2), parentRect.top + (offset.bottom / 2),
                              0, 0, SWP_NOSIZE)) {
                QCC_LogError(ER_OS_ERROR, ("SetWindowPos failed - %d", GetLastError()));
            }
            break;
        }

    case WM_COMMAND: {
            QCC_DbgTrace(("%s(hwnd=%d,msg=WM_COMMAND,wParam=0x%x,lParam=0x%x)", __FUNCTION__, hwnd, wParam, lParam));
            RequestPermissionContext* context = reinterpret_cast<RequestPermissionContext*>(GetWindowLongPtr(hwnd, GWLP_USERDATA));
            switch (LOWORD(wParam)) {
            case IDYES:
            case IDNO: {
                    if (context) {
                        context->remember = IsDlgButtonChecked(hwnd, IDC_REMEMBER);
                    } else {
                        QCC_LogError(ER_OS_ERROR, ("GetWindowLongPtr returned NULL"));
                    }
                    EndDialog(hwnd, LOWORD(wParam));
                    break;
                }

            case IDCANCEL:
                EndDialog(hwnd, IDCANCEL);
                break;
            }
            break;
        }

    default:
        return FALSE;
    }
    return TRUE;
}

QStatus RequestPermission(Plugin& plugin, const qcc::String& feature, RequestPermissionListener* listener)
{
    QStatus status = ER_OK;
    int32_t level;
    RequestPermissionContext context;
    NPError npret;
    HWND hwnd;
    INT_PTR ret;

    status = PluginData::PermissionLevel(plugin, feature, level);
    if (ER_OK != status) {
        goto exit;
    }
    QCC_DbgTrace(("Current permission level is %d", level));
    if (DEFAULT_DENIED != level) {
        listener->RequestPermissionCB(level, false);
        goto exit;
    }

    if (feature != ALLJOYN_FEATURE) {
        status = ER_FAIL;
        QCC_LogError(status, ("feature '%s' not supported", feature.c_str()));
        goto exit;
    }

    status = plugin->Origin(context.origin);
    if (ER_OK != status) {
        goto exit;
    }

    npret = NPN_GetValue(plugin->npp, NPNVnetscapeWindow, &hwnd);
    if (NPERR_NO_ERROR != npret) {
        status = ER_FAIL;
        QCC_LogError(status, ("NPN_GetValue(NPNVnetscapeWindow) failed - %d", npret));
        goto exit;
    }

    ret = DialogBoxParam(gHinstance, MAKEINTRESOURCE(IDD_PERMISSIONREQ), hwnd, reinterpret_cast<DLGPROC>(DlgProc),
                         (LPARAM)&context);
    if (ret <= 0) {
        status = ER_OS_ERROR;
        QCC_LogError(ER_OS_ERROR, ("DialogBoxParam failed - %d", GetLastError()));
        goto exit;
    }
    listener->RequestPermissionCB((IDYES == ret) ? USER_ALLOWED : USER_DENIED, context.remember);

exit:
    return status;
}

QStatus PersistentPermissionLevel(Plugin& plugin, const qcc::String& origin, int32_t& level)
{
    level = DEFAULT_DENIED;
    qcc::String filename = qcc::GetHomeDir() + "/.alljoyn_keystore/" + plugin->ToFilename(origin) + "_permission";
    QCC_DbgTrace(("filename=%s", filename.c_str()));
    qcc::FileSource source(filename);
    if (source.IsValid()) {
        source.Lock(true);
        qcc::String permission;
        source.GetLine(permission);
        source.Unlock();
        QCC_DbgHLPrintf(("Read permission '%s' from %s", permission.c_str(), filename.c_str()));
        qcc::String levelString = qcc::Trim(permission);
        if (levelString == "USER_ALLOWED") {
            level = USER_ALLOWED;
        } else if (levelString == "USER_DENIED") {
            level = USER_DENIED;
        } else if (levelString == "DEFAULT_ALLOWED") {
            level = DEFAULT_ALLOWED;
        }
    }
    return ER_OK;
}

QStatus SetPersistentPermissionLevel(Plugin& plugin, const qcc::String& origin, int32_t level)
{
    QStatus status = ER_OK;
    qcc::String filename = qcc::GetHomeDir() + "/.alljoyn_keystore/" + plugin->ToFilename(origin) + "_permission";
    qcc::FileSink sink(filename, qcc::FileSink::PRIVATE);
    if (sink.IsValid()) {
        sink.Lock(true);
        qcc::String permission;
        /*
         * Why the '\n' below?  FileSink doesn't truncate the existing file, so the line-break
         * ensures that we don't get any characters leftover from a previous write for a longer
         * string than the current one.
         */
        switch (level) {
        case USER_ALLOWED:
            permission = "USER_ALLOWED\n";
            break;

        case DEFAULT_ALLOWED:
            permission = "DEFAULT_ALLOWED\n";
            break;

        case DEFAULT_DENIED:
            permission = "DEFAULT_DENIED\n";
            break;

        case USER_DENIED:
            permission  = "USER_DENIED\n";
            break;
        }
        size_t bytesWritten;
        status = sink.PushBytes(permission.c_str(), permission.size(), bytesWritten);
        if (ER_OK == status) {
            if (permission.size() == bytesWritten) {
                QCC_DbgHLPrintf(("Wrote permission '%s' to %s", qcc::Trim(permission).c_str(), filename.c_str()));
            } else {
                status = ER_BUS_WRITE_ERROR;
                QCC_LogError(status, ("Cannot write permission to %s", filename.c_str()));
            }
        }
        sink.Unlock();
    } else {
        status = ER_BUS_WRITE_ERROR;
        QCC_LogError(status, ("Cannot write permission to %s", filename.c_str()));
    }
    return status;
}
