/*
 * This file is part of the Colobot: Gold Edition source code
 * Copyright (C) 2001-2020, Daniel Roux, EPSITEC SA & TerranovaTeam
 * http://epsitec.ch; http://colobot.info; http://github.com/colobot
 *
 * This program is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.
 * See the GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program. If not, see http://gnu.org/licenses
 */

#include "common/system/system_windows.h"

#include "common/logger.h"

#include <boost/filesystem.hpp>
#include <windows.h>


void CSystemUtilsWindows::Init()
{
    LARGE_INTEGER freq;
    QueryPerformanceFrequency(&freq);
    m_counterFrequency = freq.QuadPart;

    assert(m_counterFrequency != 0);
}

SystemDialogResult CSystemUtilsWindows::SystemDialog(SystemDialogType type, const std::string& title, const std::string& message)
{
    unsigned int windowsType = 0;
    std::wstring windowsMessage = UTF8_Decode(message);
    std::wstring windowsTitle = UTF8_Decode(title);

    switch (type)
    {
        case SDT_INFO:
        default:
            windowsType = MB_ICONINFORMATION|MB_OK;
            break;
        case SDT_WARNING:
            windowsType = MB_ICONWARNING|MB_OK;
            break;
        case SDT_ERROR:
            windowsType = MB_ICONERROR|MB_OK;
            break;
        case SDT_YES_NO:
            windowsType = MB_ICONQUESTION|MB_YESNO;
            break;
        case SDT_OK_CANCEL:
            windowsType = MB_ICONWARNING|MB_OKCANCEL;
            break;
    }

    switch (MessageBoxW(nullptr, windowsMessage.c_str(), windowsTitle.c_str(), windowsType))
    {
        case IDOK:
            return SDR_OK;
        case IDCANCEL:
            return SDR_CANCEL;
        case IDYES:
            return SDR_YES;
        case IDNO:
            return SDR_NO;
        default:
            break;
    }

    return SDR_OK;
}

void CSystemUtilsWindows::GetCurrentTimeStamp(SystemTimeStamp* stamp)
{
    LARGE_INTEGER value;
    QueryPerformanceCounter(&value);
    stamp->counterValue = value.QuadPart;
}

void CSystemUtilsWindows::InterpolateTimeStamp(SystemTimeStamp *dst, SystemTimeStamp *a, SystemTimeStamp *b, float i)
{
    dst->counterValue = a->counterValue + static_cast<long long>((b->counterValue - a->counterValue) * static_cast<double>(i));
}

long long int CSystemUtilsWindows::TimeStampExactDiff(SystemTimeStamp* before, SystemTimeStamp* after)
{
    float floatValue = static_cast<double>(after->counterValue - before->counterValue) * (1e9 / static_cast<double>(m_counterFrequency));
    return static_cast<long long>(floatValue);
}

//! Converts a wide Unicode string to an UTF8 string
std::string CSystemUtilsWindows::UTF8_Encode(const std::wstring& wstr)
{
    int size_needed = WideCharToMultiByte(CP_UTF8, 0, &wstr[0], static_cast<int>(wstr.size()), nullptr, 0, nullptr, nullptr);
    std::string strTo(size_needed, 0);
    WideCharToMultiByte(CP_UTF8, 0, &wstr[0], static_cast<int>(wstr.size()), &strTo[0], size_needed, nullptr, nullptr);
    return strTo;
}

//! Converts an UTF8 string to a wide Unicode String
std::wstring CSystemUtilsWindows::UTF8_Decode(const std::string& str)
{
    int size_needed = MultiByteToWideChar(CP_UTF8, 0, &str[0], static_cast<int>(str.size()), nullptr, 0);
    std::wstring wstrTo(size_needed, 0);
    MultiByteToWideChar(CP_UTF8, 0, &str[0], static_cast<int>(str.size()), &wstrTo[0], size_needed);
    return wstrTo;

}

std::string CSystemUtilsWindows::GetSaveDir()
{
#if PORTABLE_SAVES || DEV_BUILD
    return CSystemUtils::GetSaveDir();
#else
    std::string savegameDir;

    auto envUSERPROFILE = GetEnvVar("USERPROFILE");
    if (envUSERPROFILE.empty())
    {
        GetLogger()->Warn("Unable to find directory for saves - using default directory");
        savegameDir = CSystemUtils::GetSaveDir();
    }
    else
    {
        savegameDir = envUSERPROFILE + "\\colobot";
    }
    GetLogger()->Trace("Saved game files are going to %s\n", savegameDir.c_str());

    return savegameDir;
#endif
}

std::string CSystemUtilsWindows::GetEnvVar(const std::string& name)
{
    std::wstring wname(name.begin(), name.end());
    wchar_t* envVar = _wgetenv(wname.c_str());
    if (envVar == nullptr)
    {
        return "";
    }
    else
    {
        std::string var = UTF8_Encode(std::wstring(envVar));
        GetLogger()->Trace("Detected environment variable %s = %s\n", name.c_str(), var.c_str());
        return var;
    }
}

bool CSystemUtilsWindows::OpenPath(const std::string& path)
{
    int result = system(("start explorer \"" + boost::filesystem::path(path).make_preferred().string() + "\"").c_str());
    if (result != 0)
    {
        GetLogger()->Error("Failed to open path: %s, error code: %i\n", path.c_str(), result);
        return false;
    }
    return true;
}

bool CSystemUtilsWindows::OpenWebsite(const std::string& url)
{
    int result = system(("rundll32 url.dll,FileProtocolHandler \"" + url + "\"").c_str());
    if (result != 0)
    {
        GetLogger()->Error("Failed to open website: %s, error code: %i\n", url.c_str(), result);
        return false;
    }
    return true;
}

void CSystemUtilsWindows::Usleep(int usec)
{
   LARGE_INTEGER ft;
   ft.QuadPart = -(10 * usec); // Convert to 100 nanosecond interval, negative value indicates relative time

   HANDLE timer = CreateWaitableTimer(nullptr, TRUE, nullptr);
   SetWaitableTimer(timer, &ft, 0, nullptr, nullptr, 0);
   WaitForSingleObject(timer, INFINITE);
   CloseHandle(timer);
}
