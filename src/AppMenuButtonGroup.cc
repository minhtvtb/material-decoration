/*
 * Copyright (C) 2020 Chris Holland <zrenfire@gmail.com>
 * Copyright (C) 2016 Kai Uwe Broulik <kde@privat.broulik.de>
 * Copyright (C) 2014 by Hugo Pereira Da Costa <hugo.pereira@free.fr>
 *
 * This program is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 2 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program. If not, see <http://www.gnu.org/licenses/>.
 */

// own
#include "MinimizeButton.h"
#include "Decoration.h"
#include "TextButton.h"

// KDecoration
#include <KDecoration2/DecoratedClient>
#include <KDecoration2/DecorationButton>
#include <KDecoration2/DecorationButtonGroup>

// Qt
#include <QDebug>
#include <QLoggingCategory>
#include <QMenu>
#include <QPainter>
#include <QX11Info>

static const QLoggingCategory category("kdecoration.material");

namespace Material
{

AppMenuButtonGroup::AppMenuButtonGroup(Decoration *decoration)
    : KDecoration2::DecorationButtonGroup(decoration)
    , m_appMenuModel(nullptr)
    , m_currentIndex(-1)
{
    auto *decoratedClient = decoration->client().toStrongRef().data();
    connect(decoratedClient, &KDecoration2::DecoratedClient::hasApplicationMenuChanged,
            this, &AppMenuButtonGroup::updateAppMenuModel);
}

AppMenuButtonGroup::~AppMenuButtonGroup()
{
}

int AppMenuButtonGroup::currentIndex() const
{
    return m_currentIndex;
}

void AppMenuButtonGroup::setCurrentIndex(int set)
{
    if (m_currentIndex != set) {
        m_currentIndex = set;
        qCDebug(category) << "setCurrentIndex" << m_currentIndex;
        emit currentIndexChanged();
    }
}

void AppMenuButtonGroup::resetButtons()
{
    removeButton(KDecoration2::DecorationButtonType::Custom);
    emit menuUpdated();
}

void AppMenuButtonGroup::updateAppMenuModel()
{
    auto *deco = qobject_cast<Decoration *>(decoration());
    if (!deco) {
        return;
    }
    auto *decoratedClient = deco->client().toStrongRef().data();

    // Don't display AppMenu in modal windows.
    if (decoratedClient->isModal()) {
        resetButtons();
        return;
    }

    if (!decoratedClient->hasApplicationMenu()) {
        resetButtons();
        return;
    }

    if (m_appMenuModel) {
        // Update AppMenuModel
        qCDebug(category) << "AppMenuModel" << m_appMenuModel;

        resetButtons();

        // Populate
        for (int row = 0; row < m_appMenuModel->rowCount(); row++) {
            const QModelIndex index = m_appMenuModel->index(row, 0);
            const QString itemLabel = m_appMenuModel->data(index, AppMenuModel::MenuRole).toString();

            // https://github.com/psifidotos/applet-window-appmenu/blob/908e60831d7d68ee56a56f9c24017a71822fc02d/lib/appmenuapplet.cpp#L167
            const QVariant data = m_appMenuModel->data(index, AppMenuModel::ActionRole);
            QAction *itemAction = (QAction *)data.value<void *>();

            qCDebug(category) << "    " << itemAction;

            TextButton *b = new TextButton(deco, row, this);
            b->setText(itemLabel);
            b->setAction(itemAction);

            // Skip items with empty labels (The first item in a Gtk app)
            if (itemLabel.isEmpty()) {
                b->setVisible(false);
            }
            
            addButton(QPointer<KDecoration2::DecorationButton>(b));

        }

        emit menuUpdated();

    } else {
        // Init AppMenuModel
        qCDebug(category) << "windowId" << decoratedClient->windowId();
        WId windowId = decoratedClient->windowId();
        if (windowId != 0) {
            m_appMenuModel = new AppMenuModel(this);
            connect(m_appMenuModel, &AppMenuModel::modelReset,
                this, &AppMenuButtonGroup::updateAppMenuModel);

            qCDebug(category) << "AppMenuModel" << m_appMenuModel;
            m_appMenuModel->setWinId(windowId);
            qCDebug(category) << "AppMenuModel" << m_appMenuModel;
        }
    }
}


//* scoped pointer convenience typedef
template <typename T> using ScopedPointer = QScopedPointer<T, QScopedPointerPodDeleter>;


void AppMenuButtonGroup::trigger(int buttonIndex, KDecoration2::DecorationButton* button) {
    qCDebug(category) << "AppMenuButtonGroup::trigger" << buttonIndex;

    // https://github.com/psifidotos/applet-window-appmenu/blob/908e60831d7d68ee56a56f9c24017a71822fc02d/lib/appmenuapplet.cpp#L167
    QMenu *actionMenu = nullptr;

    const QModelIndex modelIndex = m_appMenuModel->index(buttonIndex, 0);
    const QVariant data = m_appMenuModel->data(modelIndex, AppMenuModel::ActionRole);
    QAction *itemAction = (QAction *)data.value<void *>();
    qCDebug(category) << "    action" << itemAction;

    if (itemAction) {
        actionMenu = itemAction->menu();
        qCDebug(category) << "    menu" << actionMenu;
    }

    const auto *deco = qobject_cast<Decoration *>(decoration());
    // if (actionMenu && deco) {
    //     auto *decoratedClient = deco->client().toStrongRef().data();
    //     actionMenu->setPalette(decoratedClient->palette());
    // }

    if (actionMenu && deco) {
        QRectF buttonRect = button ? button->geometry() : geometry();
        QPoint position = buttonRect.topLeft().toPoint();
        qCDebug(category) << "    geometry" << geometry();
        qCDebug(category) << "    position" << position;

        auto *decoratedClient = deco->client().toStrongRef().data();
        WId windowId = decoratedClient->windowId();
        qCDebug(category) << "    windowId" << windowId;

        //--- From: BreezeSizeGrip.cpp
        /*
        get root position matching position
        need to use xcb because the embedding of the widget
        breaks QT's mapToGlobal and other methods
        */
        QPoint rootPosition(position);
        auto connection( QX11Info::connection() );
        xcb_get_geometry_cookie_t cookie( xcb_get_geometry( connection, windowId ) );
        ScopedPointer<xcb_get_geometry_reply_t> reply( xcb_get_geometry_reply( connection, cookie, nullptr ) );
        if (reply) {
            // translate coordinates
            xcb_translate_coordinates_cookie_t coordCookie( xcb_translate_coordinates(
                connection, windowId, reply.data()->root,
                -reply.data()->border_width,
                -reply.data()->border_width ) );

            ScopedPointer< xcb_translate_coordinates_reply_t> coordReply( xcb_translate_coordinates_reply( connection, coordCookie, nullptr ) );

            if (coordReply) {
                rootPosition.rx() += coordReply.data()->dst_x;
                rootPosition.ry() += coordReply.data()->dst_y;
            }
        }
        qCDebug(category) << "    rootPosition" << rootPosition;

        // button release event
        // xcb_button_release_event_t releaseEvent;
        // memset(&releaseEvent, 0, sizeof(releaseEvent));

        // releaseEvent.response_type = XCB_BUTTON_RELEASE;
        // releaseEvent.event =  windowId;
        // releaseEvent.child = XCB_WINDOW_NONE;
        // releaseEvent.root = QX11Info::appRootWindow();
        // releaseEvent.event_x = position.x();
        // releaseEvent.event_y = position.y();
        // releaseEvent.root_x = rootPosition.x();
        // releaseEvent.root_y = rootPosition.y();
        // releaseEvent.detail = XCB_BUTTON_INDEX_1;
        // releaseEvent.state = XCB_BUTTON_MASK_1;
        // releaseEvent.time = XCB_CURRENT_TIME;
        // releaseEvent.same_screen = true;
        // xcb_send_event( connection, false, windowId, XCB_EVENT_MASK_BUTTON_RELEASE, reinterpret_cast<const char*>(&releaseEvent));

        // xcb_ungrab_pointer( connection, XCB_TIME_CURRENT_TIME );
        //---

        actionMenu->popup(rootPosition);

        QMenu *oldMenu = m_currentMenu;
        m_currentMenu = actionMenu;

        if (oldMenu && oldMenu != actionMenu) {
            // Don't reset the currentIndex when another menu is already shown
            disconnect(oldMenu, &QMenu::aboutToHide, this, &AppMenuButtonGroup::onMenuAboutToHide);
            oldMenu->hide();
        }

        setCurrentIndex(buttonIndex);

        // FIXME TODO connect only once
        connect(actionMenu, &QMenu::aboutToHide, this, &AppMenuButtonGroup::onMenuAboutToHide, Qt::UniqueConnection);
    }
}

void AppMenuButtonGroup::onMenuAboutToHide()
{
    setCurrentIndex(-1);
}

} // namespace Material
