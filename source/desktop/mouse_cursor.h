//
// Aspia Project
// Copyright (C) 2018 Dmitry Chapyshev <dmitry@aspia.ru>
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

#ifndef ASPIA_DESKTOP__MOUSE_CURSOR_H
#define ASPIA_DESKTOP__MOUSE_CURSOR_H

#include <QPoint>
#include <QSize>

#include <memory>

namespace desktop {

class MouseCursor
{
public:
    MouseCursor(std::unique_ptr<uint8_t[]> data,
                const QSize& size,
                const QPoint& hotspot);
    ~MouseCursor() = default;

    const QSize& size() const { return size_; }
    const QPoint& hotSpot() const { return hotspot_; }
    uint8_t* data() const { return data_.get(); }

    int stride() const;

    bool isEqual(const MouseCursor& other);

private:
    std::unique_ptr<uint8_t[]> const data_;
    const QSize size_;
    const QPoint hotspot_;
};

} // namespace desktop

#endif // ASPIA_DESKTOP__MOUSE_CURSOR_H