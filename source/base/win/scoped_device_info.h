//
// Aspia Project
// Copyright (C) 2016-2022 Dmitry Chapyshev <dmitry@aspia.ru>
//
// This program is free software: you can redistribute it and/or modify
// it under the terms of the GNU General Public License as published by
// the Free Software Foundation, either version 3 of the License, or
// (at your option) any later version.
//
// This program is distributed in the hope that it will be useful,
// but WITHOUT ANY WARRANTY; without even the implied warranty of
// MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the
// GNU General Public License for more details.
//
// You should have received a copy of the GNU General Public License
// along with this program. If not, see <https://www.gnu.org/licenses/>.
//

#ifndef BASE__WIN__SCOPED_DEVICE_INFO_H
#define BASE__WIN__SCOPED_DEVICE_INFO_H

#include "base/macros_magic.h"

#include <Windows.h>
#include <setupapi.h>

namespace base::win {

class ScopedDeviceInfo
{
public:
    ScopedDeviceInfo() = default;

    explicit ScopedDeviceInfo(HDEVINFO device_info)
        : device_info_(device_info)
    {
        // Nothing
    }

    ~ScopedDeviceInfo()
    {
        close();
    }

    HDEVINFO get() const
    {
        return device_info_;
    }

    operator HDEVINFO()
    {
        return device_info_;
    }

    void reset(HDEVINFO device_info = INVALID_HANDLE_VALUE)
    {
        close();
        device_info_ = device_info;
    }

    HDEVINFO release()
    {
        HDEVINFO device_info = device_info_;
        device_info_ = INVALID_HANDLE_VALUE;
        return device_info;
    }

    bool isValid() const
    {
        return device_info_ != INVALID_HANDLE_VALUE;
    }

private:
    void close()
    {
        if (isValid())
        {
            SetupDiDestroyDeviceInfoList(device_info_);
            device_info_ = INVALID_HANDLE_VALUE;
        }
    }

    HDEVINFO device_info_ = INVALID_HANDLE_VALUE;

    DISALLOW_COPY_AND_ASSIGN(ScopedDeviceInfo);
};

} // namespace base::win

#endif // BASE__WIN__SCOPED_DEVICE_INFO_H