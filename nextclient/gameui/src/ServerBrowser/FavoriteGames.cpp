#include "FavoriteGames.h"

#include "ServerContextMenu.h"
#include "ServerListCompare.h"
#include "ServerBrowserDialog.h"
#include "DialogAddServer.h"
#include "InternetGames.h"
#include "FileSystem.h"
#include "utlbuffer.h"

#include <vgui/ISchemeNext.h>
#include <vgui/ISystem.h>
#include <vgui/IVGui.h>
#include <KeyValues.h>

#include <vgui_controls/Button.h>
#include <vgui_controls/ListPanel.h>
#include <vgui_controls/ImagePanel.h>
#include <vgui_controls/MessageBox.h>

using namespace vgui2;

CFavoriteGames::CFavoriteGames(vgui2::Panel *parent) :
    CBaseGamesPage(parent, "FavoriteGames")
{

}

CFavoriteGames::~CFavoriteGames()
{
}

void CFavoriteGames::OnPageShow()
{
    if (!ServerBrowserDialog().IsVisible())
        return;

    GetNewServerList();
}

void CFavoriteGames::OnPageHide()
{
    StopRefresh(CancelQueryReason::PageClosed);
}

void CFavoriteGames::OnViewGameInfo()
{
    if (!m_pGameList->GetSelectedItemsCount())
        return;

    int serverID = m_pGameList->GetItemUserData(m_pGameList->GetSelectedItem(0));

    ServerBrowserDialog().OpenGameInfoDialog(this, serverID);
}

bool CFavoriteGames::SupportsItem(InterfaceItem item)
{
    switch (item)
    {
        case InterfaceItem::Filters:
        case InterfaceItem::AddServer:
        case InterfaceItem::AddCurrentServer: return true;
    }

    return false;
}

void CFavoriteGames::StartRefresh()
{
    StopRefresh(CancelQueryReason::NewQuery);
    SetRefreshing(true);

    m_Servers.StartRefresh();
}

void CFavoriteGames::GetNewServerList()
{
    if (!IsVisible())
        return;

    StopRefresh(CancelQueryReason::NewQuery);

    m_pGameList->DeleteAllItems();

    m_Servers.Clear();
    m_Servers.RequestFavorites(GetFilter(), GetFilterCount());

    UpdateRefreshStatusText();
    SetRefreshing(true);
}

void CFavoriteGames::StopRefresh(CancelQueryReason reason)
{
    m_Servers.StopRefresh(reason);

    CBaseGamesPage::StopRefresh(reason);
}

GuiConnectionSource CFavoriteGames::GetConnectionSource()
{
    return GuiConnectionSource::ServersFavorites;
}

void CFavoriteGames::AddNewServer(uint32_t ip, uint16_t port)
{
    bool result = SteamMatchmaking()->AddFavoriteGame(SteamUtils()->GetAppID(), ip, port, port, k_unFavoriteFlagFavorite, 0);

    if (result && IsVisible())
        GetNewServerList();
}

void CFavoriteGames::OnAddCurrentServer()
{
    servernetadr_t &addr = ServerBrowserDialog().GetCurrentConnectedServer();

    AddNewServer(addr.GetIP(), addr.GetConnectionPort());
}

void CFavoriteGames::RefreshComplete()
{
    SetRefreshing(false);
    UpdateFilterSettings();
    UpdateRefreshStatusText();

    if (IsVisible())
        m_pGameList->SortList();
}

void CFavoriteGames::ServerFailedToRespond(serveritem_t &server)
{
    ServerResponded(server);
}

void CFavoriteGames::OnOpenContextMenu(int itemID)
{
    CServerContextMenu *menu = ServerBrowserDialog().GetContextMenu(m_pGameList);

    if (m_pGameList->GetSelectedItemsCount())
    {
        int serverID = m_pGameList->GetItemUserData(m_pGameList->GetSelectedItem(0));

        menu->ShowMenu(this, serverID, true, true, true, false);
        menu->AddMenuItem("RemoveServer", "#ServerBrowser_RemoveServerFromFavorites", new KeyValues("RemoveFromFavorites"), this);
    }
    else
        menu->ShowMenu(this, (unsigned int)-1, false, false, false, false);

    menu->AddMenuItem("AddServerByName", "#ServerBrowser_AddServerByIP", new KeyValues("AddServerByName"), this);
}

void CFavoriteGames::OnRefreshServer(int serverID)
{
    if (m_Servers.IsRefreshing())
        return;

    m_Servers.StartRefreshServer(serverID);
}

void CFavoriteGames::OnRemoveFromFavorites()
{
    while (m_pGameList->GetSelectedItemsCount() > 0)
    {
        int itemID = m_pGameList->GetSelectedItem(0);
        unsigned int serverID = m_pGameList->GetItemData(itemID)->userData;

        if (serverID >= m_Servers.ServerCount())
            continue;

        serveritem_t &server = m_Servers.GetServer(serverID);

        uint32_t ip = server.gs.m_NetAdr.GetIP();
        uint16_t port = server.gs.m_NetAdr.GetConnectionPort();

        SteamMatchmaking()->RemoveFavoriteGame(SteamUtils()->GetAppID(), ip, port, port, k_unFavoriteFlagFavorite);

        if (m_pGameList->IsValidItemID(server.listEntryID))
        {
            m_pGameList->RemoveItem(server.listEntryID);
            server.listEntryID = GetInvalidServerListID();
        }
    }

    UpdateRefreshStatusText();

    InvalidateLayout();
    Repaint();
}

void CFavoriteGames::OnAddServerByName()
{
    auto *dlg = new CDialogAddServer(&ServerBrowserDialog());
    dlg->MoveToCenterOfScreen();
    dlg->DoModal();
}

void CFavoriteGames::OnCommand(const char *command)
{
    if (!Q_stricmp(command, "AddServerByName"))
        OnAddServerByName();
    else if (!Q_stricmp(command, "AddCurrentServer"))
        OnAddCurrentServer();
    else
        BaseClass::OnCommand(command);
}

