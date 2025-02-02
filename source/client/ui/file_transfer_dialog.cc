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

#include "client/ui/file_transfer_dialog.h"

#include "base/logging.h"
#include "client/file_transfer_proxy.h"
#include "client/file_transfer_window_proxy.h"
#include "client/ui/file_error_code.h"
#include "qt_base/application.h"

#include <QCloseEvent>
#include <QPushButton>
#include <QMessageBox>

namespace client {

FileTransferDialog::FileTransferDialog(QWidget* parent)
    : QDialog(parent),
      transfer_window_proxy_(std::make_shared<FileTransferWindowProxy>(
          qt_base::Application::uiTaskRunner(), this))
{
    ui.setupUi(this);
    setFixedHeight(sizeHint().height());

    QPushButton* cancel_button = ui.button_box->button(QDialogButtonBox::StandardButton::Cancel);
    if (cancel_button)
        cancel_button->setText(tr("Cancel"));

    ui.progress_total->setRange(0, 0);
    ui.progress_current->setRange(0, 0);

    connect(ui.button_box, &QDialogButtonBox::clicked, this, &FileTransferDialog::close);

    label_metrics_ = std::make_unique<QFontMetrics>(ui.label_source->font());
}

FileTransferDialog::~FileTransferDialog()
{
    transfer_window_proxy_->dettach();
}

void FileTransferDialog::start(std::shared_ptr<FileTransferProxy> transfer_proxy)
{
    transfer_proxy_ = std::move(transfer_proxy);
    DCHECK(transfer_proxy_);

    show();
    activateWindow();
}

void FileTransferDialog::stop()
{
    finished_ = true;
    close();
}

void FileTransferDialog::setCurrentItem(
    const std::string& source_path, const std::string& target_path)
{
    if (task_queue_building_)
    {
        task_queue_building_ = false;
        ui.label_task->setText(tr("Current Task: Copying items."));

        ui.progress_total->setRange(0, 100);
        ui.progress_current->setRange(0, 100);
    }

    QString source_text = label_metrics_->elidedText(
        tr("From: %1").arg(QString::fromStdString(source_path)),
        Qt::ElideMiddle,
        ui.label_source->width());

    QString target_text = label_metrics_->elidedText(
        tr("To: %1").arg(QString::fromStdString(target_path)),
        Qt::ElideMiddle,
        ui.label_target->width());

    ui.label_source->setText(source_text);
    ui.label_target->setText(target_text);
}

void FileTransferDialog::setCurrentProgress(int total, int current)
{
    ui.progress_total->setValue(total);
    ui.progress_current->setValue(current);
}

void FileTransferDialog::errorOccurred(const FileTransfer::Error& error)
{
    QMessageBox* dialog = new QMessageBox(this);

    dialog->setWindowTitle(tr("Warning"));
    dialog->setIcon(QMessageBox::Warning);
    dialog->setText(errorToMessage(error));

    QAbstractButton* skip_button = nullptr;
    QAbstractButton* skip_all_button = nullptr;
    QAbstractButton* replace_button = nullptr;
    QAbstractButton* replace_all_button = nullptr;

    const uint32_t available_actions = error.availableActions();

    if (available_actions & FileTransfer::Error::ACTION_SKIP)
        skip_button = dialog->addButton(tr("Skip"), QMessageBox::ButtonRole::ActionRole);

    if (available_actions & FileTransfer::Error::ACTION_SKIP_ALL)
        skip_all_button = dialog->addButton(tr("Skip All"), QMessageBox::ButtonRole::ActionRole);

    if (available_actions & FileTransfer::Error::ACTION_REPLACE)
        replace_button = dialog->addButton(tr("Replace"), QMessageBox::ButtonRole::ActionRole);

    if (available_actions & FileTransfer::Error::ACTION_REPLACE_ALL)
        replace_all_button = dialog->addButton(tr("Replace All"), QMessageBox::ButtonRole::ActionRole);

    if (available_actions & FileTransfer::Error::ACTION_ABORT)
        dialog->addButton(tr("Abort"), QMessageBox::ButtonRole::ActionRole);

    connect(dialog, &QMessageBox::buttonClicked, this, [&](QAbstractButton* button)
    {
        if (button != nullptr)
        {
            if (button == skip_button)
            {
                transfer_proxy_->setAction(error.type(), FileTransfer::Error::ACTION_SKIP);
                return;
            }

            if (button == skip_all_button)
            {
                transfer_proxy_->setAction(error.type(), FileTransfer::Error::ACTION_SKIP_ALL);
                return;
            }

            if (button == replace_button)
            {
                transfer_proxy_->setAction(error.type(), FileTransfer::Error::ACTION_REPLACE);
                return;
            }

            if (button == replace_all_button)
            {
                transfer_proxy_->setAction(error.type(), FileTransfer::Error::ACTION_REPLACE_ALL);
                return;
            }
        }

        transfer_proxy_->setAction(error.type(), FileTransfer::Error::ACTION_ABORT);
    });

    connect(dialog, &QMessageBox::finished, dialog, &QMessageBox::deleteLater);

    dialog->exec();
}

void FileTransferDialog::keyPressEvent(QKeyEvent* event)
{
    // If the user presses the Esc key in a dialog, QDialog::reject() will be called. This will
    // cause the window to close: The close event cannot be ignored.
    // We do not allow pressing Esc to cause regular behavior. We intercept pressing and we cause
    // closing of dialog.
    if (event->key() == Qt::Key_Escape)
    {
        close();
        return;
    }

    QDialog::keyPressEvent(event);
}

void FileTransferDialog::closeEvent(QCloseEvent* event)
{
    if (finished_)
    {
        event->accept();
        accept();
    }
    else
    {
        event->ignore();

        if (!closing_)
        {
            closing_ = true;

            ui.label_task->setText(tr("Current Task: Cancel transfer of files."));
            ui.button_box->setDisabled(true);

            transfer_proxy_->stop();
        }
    }
}

QString FileTransferDialog::errorToMessage(const FileTransfer::Error& error)
{
    switch (error.type())
    {
        case FileTransfer::Error::Type::QUEUE:
        {
            return tr("An error occurred while building the file queue for copying");
        }

        case FileTransfer::Error::Type::CREATE_DIRECTORY:
        {
            return tr("Failed to create directory \"%1\": %2")
                .arg(QString::fromStdString(error.path()))
                .arg(fileErrorToString(error.code()));
        }

        case FileTransfer::Error::Type::CREATE_FILE:
        case FileTransfer::Error::Type::ALREADY_EXISTS:
        {
            return tr("Failed to create file \"%1\": %2")
                .arg(QString::fromStdString(error.path()))
                .arg(fileErrorToString(error.code()));
        }

        case FileTransfer::Error::Type::OPEN_FILE:
        {
            return tr("Failed to open file \"%1\": %2")
                .arg(QString::fromStdString(error.path()))
                .arg(fileErrorToString(error.code()));
        }

        case FileTransfer::Error::Type::WRITE_FILE:
        {
            return tr("Failed to write file \"%1\": %2")
                .arg(QString::fromStdString(error.path()))
                .arg(fileErrorToString(error.code()));
        }

        case FileTransfer::Error::Type::READ_FILE:
        {
            return tr("Failed to read file \"%1\": %2")
                .arg(QString::fromStdString(error.path()))
                .arg(fileErrorToString(error.code()));
        }

        default:
        {
            return tr("Unknown error type while copying files");
        }
    }
}

} // namespace client
