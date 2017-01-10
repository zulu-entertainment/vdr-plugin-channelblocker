/*
Copyright (C) 2008 Marc Wernecke
Based on noepgmenu-0.0.6.beta2 from Christian Wieninger

This program is free software; you can redistribute it and/or
modify it under the terms of the GNU General Public License
as published by the Free Software Foundation; either version 2
of the License, or (at your option) any later version.

This program is distributed in the hope that it will be useful,
but WITHOUT ANY WARRANTY; without even the implied warranty of
MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
GNU General Public License for more details.

You should have received a copy of the GNU General Public License
along with this program; if not, write to the Free Software
Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA 02110-1301 USA
Or, point your browser to http://www.gnu.org/licenses/old-licenses/gpl-2.0.html

*/

#include <vector>
#include <map>
#include <string>
#include <linux/dvb/frontend.h>
#include <linux/dvb/version.h>
#include <vdr/plugin.h>
#include <vdr/device.h>

static const char *VERSION        = "0.0.6";
static const char *DESCRIPTION    = trNOOP("A menu for the ChannelBlocker-Patch");
static const char *MAINMENUENTRY  = trNOOP("Block Channels");

#define CHNUMWIDTH  (numdigits(Channels.MaxNumber()) + 1)

typedef enum { OpenProvider, BlockedProvider, BlockModeToggle } BlockMode;

// --- cConfigChannelBlocker ------------------------------------------------------
class cConfigChannelBlocker
{
  std::map<const cChannel*, BlockMode> channelModes;
public:
  std::string oldChannelBlockerList;
  int hidemenu;
  int filterlist;
  cConfigChannelBlocker(void) : hidemenu(0), filterlist(0) {}

  int HasChange;

  void InitChannelSettings()
  {
    for (const cChannel *ch = Channels.First(); ch; ch = Channels.Next(ch))
        channelModes[ch] = allowedChannel(ch->GetChannelID()) ? OpenProvider : BlockedProvider;
  }

  bool allowedChannel(tChannelID chID)
  {
    if (Setup.ChannelBlocker == 5)  // whitelist
       return strstr(Setup.ChannelBlockerList, chID.ToString()) != NULL;
    else
       return strstr(Setup.ChannelBlockerList, chID.ToString()) == NULL;
  }

  BlockMode GetChannelMode(const cChannel* channel)
  {
    std::map<const cChannel*, BlockMode>::iterator i = channelModes.find(channel);
    if (i != channelModes.end())
       return i->second;
    else
       return OpenProvider;
  }

  void SetChannelMode(const cChannel* channel, BlockMode mode, bool save = true)
  {
    channelModes[channel] = mode;
    if (save) {
       std::string chList;
       for (const cChannel *ch = Channels.First(); ch; ch = Channels.Next(ch))
           if (!ch->GroupSep() && GetChannelMode(ch) == BlockedProvider)
              chList += std::string(chList.empty() ? "" : " ") + *ch->GetChannelID().ToString();
       if (Setup.ChannelBlockerList) free(Setup.ChannelBlockerList);
       Setup.ChannelBlockerList = strdup(chList.c_str());
       }
  }
};

cConfigChannelBlocker ConfigChannelBlocker;


// --- cMenuChannelItemChannelBlocker ----------------------------------------------
class cMenuChannelItemChannelBlocker : public cOsdItem {
private:
  const cChannel *channel;
public:
  cMenuChannelItemChannelBlocker(const cChannel *Channel) : channel(Channel) { Set(); }
  void Set();
  virtual int Compare(const cListObject &ListObject) const;
  const cChannel *Channel(void) { return channel; }
  };

void cMenuChannelItemChannelBlocker::Set()
{
  if (!channel->GroupSep())
     SetText(cString::sprintf("%d\t%s\t%s", channel->Number(), channel->Name(),
                               ConfigChannelBlocker.GetChannelMode(channel) == OpenProvider ? tr("open") : tr("blocked")));
  else
     SetText(cString::sprintf("---\t%s ------------------------------------------------------------------------------------------", channel->Name()));
}

int cMenuChannelItemChannelBlocker::Compare(const cListObject &ListObject) const
{
  cMenuChannelItemChannelBlocker *p = (cMenuChannelItemChannelBlocker *)&ListObject;
  return channel->Number() - p->channel->Number();
}


// --- cMenuEditChannelBlocker --------------------------------------------------

class cMenuEditChannelBlocker : public cOsdMenu {
private:
  int t_hidemenu;
  int t_filterlist;
  int t_ChannelBlocker;
  int t_ChannelBlockerMode;

  const char *Filters[4];
  const char *ChannelBlockers[7];
  const char *ChannelBlockerModes[4];
protected:
  void AddCategory(const char *Title);
public:
  cMenuEditChannelBlocker(void);
  ~cMenuEditChannelBlocker();
  virtual eOSState ProcessKey(eKeys Key);
};

void cMenuEditChannelBlocker::AddCategory(const char *Title) {
  cOsdItem *item = new cOsdItem(cString::sprintf("--- %s ------------------------------------------------------------------------------------------", Title));
  item->SetSelectable(false);
  Add(item);
}

cMenuEditChannelBlocker::cMenuEditChannelBlocker(void) : cOsdMenu(trVDR("Setup"), 30) {

  t_hidemenu = ConfigChannelBlocker.hidemenu;
  t_filterlist = ConfigChannelBlocker.filterlist;
  t_ChannelBlocker = Setup.ChannelBlocker;
  t_ChannelBlockerMode = Setup.ChannelBlockerMode;

  ChannelBlockers[0] = trVDR("none");
  ChannelBlockers[1] = trVDR("qam256");
  ChannelBlockers[2] = trVDR("dvb-c");
  ChannelBlockers[3] = trVDR("dvb-s");
  ChannelBlockers[4] = trVDR("blacklist");
  ChannelBlockers[5] = trVDR("whitelist");
  ChannelBlockers[6] = trVDR("all");

  ChannelBlockerModes[0] = trVDR("none");
  ChannelBlockerModes[1] = trVDR("has decoder");
  ChannelBlockerModes[2] = trVDR("is primary");
  ChannelBlockerModes[3] = trVDR("has decoder + is primary");

  Filters[0] = trVDR("all");
  Filters[1] = trVDR("qam256");
  Filters[2] = trVDR("dvb-c");
  Filters[3] = trVDR("dvb-s");

  // menu
  AddCategory(trVDR("VDR"));
  Add(new cMenuEditStraItem(trVDR("Setup.DVB$Channel Blocker"), &t_ChannelBlocker, 7, ChannelBlockers));
  Add(new cMenuEditStraItem(trVDR("Setup.DVB$Channel Blocker Filter Mode"), &t_ChannelBlockerMode, 4, ChannelBlockerModes));

  AddCategory(trVDR("Plugin"));
  Add(new cMenuEditBoolItem(tr("Hide main menu entry"), &t_hidemenu, trVDR("no"), trVDR("yes")));
  Add(new cMenuEditStraItem(tr("Shown channels"), &t_filterlist, 4, Filters));
  }

cMenuEditChannelBlocker::~cMenuEditChannelBlocker() {
  }

eOSState cMenuEditChannelBlocker::ProcessKey(eKeys Key) {
  eOSState state = cOsdMenu::ProcessKey(Key);
  if (state == osUnknown) {
     switch (Key) {
        case kOk: {
           ConfigChannelBlocker.hidemenu = t_hidemenu;
           ConfigChannelBlocker.filterlist = t_filterlist;
           Setup.ChannelBlocker = t_ChannelBlocker;
           Setup.ChannelBlockerMode = t_ChannelBlockerMode;
           Setup.Save();
           ConfigChannelBlocker.HasChange = 1;
           state = osBack;
           break;
           }
        default: {
           state = osContinue;
           break;
           }
        }
     }
  return state;
}

// --- cMenuChannelsChannelBlocker ---------------------------------------------------------
class cMenuChannelsChannelBlocker : public cOsdMenu {
private:
  void Set(void);
  void SetMenuTitle(void);
  const cChannel *GetChannel(int Index);
  const char *ChannelBlockers[7];
  cChannel mainGroup;
protected:
  eOSState SetMode(int index, BlockMode mode);
  eOSState SwitchCh(int index);
public:
  cMenuChannelsChannelBlocker(void);
  virtual eOSState ProcessKey(eKeys Key);
  };

cMenuChannelsChannelBlocker::cMenuChannelsChannelBlocker(void) : cOsdMenu(tr(MAINMENUENTRY), CHNUMWIDTH, 30)
{
  ConfigChannelBlocker.InitChannelSettings();
  // create a main channel group
  mainGroup.Parse(tr(":Main channels"));
  Set();
  SetHelp(tr("Button$Block"), tr("Button$Open"), tr("Button$Toggle"), tr("Button$Setup"));
  SetMenuTitle();
}

void cMenuChannelsChannelBlocker::SetMenuTitle(void)
{
  ChannelBlockers[0] = trVDR("none");
  ChannelBlockers[1] = trVDR("qam256");
  ChannelBlockers[2] = trVDR("dvb-c");
  ChannelBlockers[3] = trVDR("dvb-s");
  ChannelBlockers[4] = trVDR("blacklist");
  ChannelBlockers[5] = trVDR("whitelist");
  ChannelBlockers[6] = trVDR("all");

  SetTitle(cString::sprintf("%s - %s", tr(MAINMENUENTRY), ChannelBlockers[Setup.ChannelBlocker]));
}

void cMenuChannelsChannelBlocker::Set(void)
{
  const cChannel *currentChannel = GetChannel(Current());
  Clear();
  if (Channels.First() && !Channels.First()->GroupSep()) // add a main group if there's none
     Add(new cMenuChannelItemChannelBlocker(&mainGroup));
  for (const cChannel *channel = Channels.First(); channel; channel = Channels.Next(channel))
      if (*channel->Name()) {
         if ((channel->GroupSep()) ||
            (ConfigChannelBlocker.filterlist == 0) ||
#if   VDRVERSNUM < 10700 && DVB_API_VERSION < 5
            (ConfigChannelBlocker.filterlist == 1 && fe_modulation_t(channel->Modulation()) == QAM_256) ||
#elif VDRVERSNUM < 10702 && DVB_API_VERSION < 5
            (ConfigChannelBlocker.filterlist == 1 && dvbfe_modulation(channel->Modulation()) == DVBFE_MOD_QAM256) ||
#else
            (ConfigChannelBlocker.filterlist == 1 && channel->Modulation() == QAM_256) ||
#endif
            (ConfigChannelBlocker.filterlist == 2 && cSource::IsCable(channel->Source())) ||
            (ConfigChannelBlocker.filterlist == 3 && cSource::IsSat(channel->Source())))

            Add(new cMenuChannelItemChannelBlocker(channel), channel == currentChannel);
         }
}

const cChannel *cMenuChannelsChannelBlocker::GetChannel(int Index)
{
  cMenuChannelItemChannelBlocker *p = (cMenuChannelItemChannelBlocker *)Get(Index);
  return p ? p->Channel() : NULL;
}

eOSState cMenuChannelsChannelBlocker::SetMode(int index, BlockMode mode)
{
  BlockMode newMode = mode;
  const cChannel* channel = GetChannel(index);
  if (!channel) return osContinue;
  std::vector<const cChannel*> channelList;
  bool singleUpdate = true;
  if (!channel->GroupSep())
     channelList.push_back(channel);
  else {
     singleUpdate = false;
     for (int c = index+1; c < Count()-1; c++) {
         const cChannel* ch = GetChannel(c);
         if (ch->GroupSep()) break;
         channelList.push_back(ch);
         }
     }
  for (std::vector<const cChannel*>::iterator i = channelList.begin(); i != channelList.end(); ++i) {
      if (mode == BlockModeToggle)
         newMode = ConfigChannelBlocker.GetChannelMode(*i) == OpenProvider ? BlockedProvider : OpenProvider;
      std::vector<const cChannel*>::iterator next = i;
      next++;
      ConfigChannelBlocker.SetChannelMode(*i, newMode, next == channelList.end());
      }

  if (!singleUpdate)
     Set();
  else
     RefreshCurrent();

  Display();
  return osContinue;
}

eOSState cMenuChannelsChannelBlocker::SwitchCh(int index)
{
  const cChannel* channel = GetChannel(index);
  if (!channel)
     return osContinue;
  if (!channel->GroupSep())
     Channels.SwitchTo(channel->Number());
  return osContinue;
}

eOSState cMenuChannelsChannelBlocker::ProcessKey(eKeys Key)
{
  eOSState state = cOsdMenu::ProcessKey(Key);

  if (ConfigChannelBlocker.HasChange) {
     ConfigChannelBlocker.HasChange = 0;
     ConfigChannelBlocker.InitChannelSettings();
     Set();
     SetMenuTitle();
     Display();
     }

  switch (state) {
     case osBack:
        if (Setup.ChannelBlockerList) free(Setup.ChannelBlockerList);
        Setup.ChannelBlockerList = strdup(ConfigChannelBlocker.oldChannelBlockerList.c_str());
        state = osBack;
        break;
     default:
        if (state == osUnknown) {
           switch (Key) {
              case k0:      state = SwitchCh(Current());
                            break;
              case kOk:     Setup.Save();
                            state = osBack;
                            break;
              case kYellow: state = SetMode(Current(), BlockModeToggle);
                            break;
              case kRed:
              case kGreen:  state = SetMode(Current(), Key == kGreen ? OpenProvider : BlockedProvider);
                            break;
              case kBlue:   state = AddSubMenu(new cMenuEditChannelBlocker());
                            break;
              default:      break;
              }
           }
     }
  return state;
}

// --- cMenuSetupChannelBlocker --------------------------------------------------
class cMenuSetupChannelBlocker : public cMenuSetupPage {
  private:
    virtual void Setup(void);
    cConfigChannelBlocker data;
    const char *Filters[4];
  protected:
    virtual eOSState ProcessKey(eKeys Key);
    virtual void Store(void);
    void Set(void);
  public:
    cMenuSetupChannelBlocker(void);
};

cMenuSetupChannelBlocker::cMenuSetupChannelBlocker(void)
{
  Setup();
}

void cMenuSetupChannelBlocker::Setup(void)
{
  data = ConfigChannelBlocker;
  Set();
}

void cMenuSetupChannelBlocker::Set()
{
  int current = Current();
  Clear();

  Filters[0] = trVDR("all");
  Filters[1] = trVDR("qam256");
  Filters[2] = trVDR("dvb-c");
  Filters[3] = trVDR("dvb-s");

  Add(new cMenuEditBoolItem(tr("Hide main menu entry"), &data.hidemenu, trVDR("no"), trVDR("yes")));
  Add(new cMenuEditStraItem(tr("Shown channels"), &data.filterlist, 4, Filters));
  SetCurrent(Get(current));
  Display();
}

void cMenuSetupChannelBlocker::Store(void)
{
    ConfigChannelBlocker = data;

    SetupStore("HideMenu", ConfigChannelBlocker.hidemenu);
    SetupStore("FilterList", ConfigChannelBlocker.filterlist);
}

eOSState cMenuSetupChannelBlocker::ProcessKey(eKeys Key)
{
    return cMenuSetupPage::ProcessKey(Key);
}

class cPluginChannelBlocker : public cPlugin {
private:
public:
  cPluginChannelBlocker(void);
  virtual ~cPluginChannelBlocker();
  virtual const char *Version(void) { return VERSION; }
  virtual const char *Description(void) { return tr(DESCRIPTION); }
  virtual const char *CommandLineHelp(void);
  virtual bool ProcessArgs(int argc, char *argv[]);
  virtual bool Initialize(void);
  virtual bool Start(void);
  virtual void Stop(void);
  virtual void Housekeeping(void);
  virtual const char *MainMenuEntry(void) { return (ConfigChannelBlocker.hidemenu ? NULL : tr(MAINMENUENTRY)); }
  virtual cOsdObject *MainMenuAction(void);
  virtual cMenuSetupPage *SetupMenu(void);
  virtual bool SetupParse(const char *Name, const char *Value);
  virtual bool Service(const char *Id, void *Data = NULL);
  virtual const char **SVDRPHelpPages(void);
  virtual cString SVDRPCommand(const char *Command, const char *Option, int &ReplyCode);
  };

cPluginChannelBlocker::cPluginChannelBlocker(void)
{
}

cPluginChannelBlocker::~cPluginChannelBlocker()
{
}

const char *cPluginChannelBlocker::CommandLineHelp(void)
{
  return NULL;
}

bool cPluginChannelBlocker::ProcessArgs(int argc, char *argv[])
{
  return true;
}

bool cPluginChannelBlocker::Initialize(void)
{
  return true;
}

bool cPluginChannelBlocker::Start(void)
{
  return true;
}

void cPluginChannelBlocker::Stop(void)
{
}

void cPluginChannelBlocker::Housekeeping(void)
{
}

cOsdObject *cPluginChannelBlocker::MainMenuAction(void)
{
    ConfigChannelBlocker.oldChannelBlockerList = strdup(Setup.ChannelBlockerList);
    return new cMenuChannelsChannelBlocker;
}

cMenuSetupPage *cPluginChannelBlocker::SetupMenu(void)
{
  return new cMenuSetupChannelBlocker;
}

bool cPluginChannelBlocker::SetupParse(const char *Name, const char *Value)
{
    if (!strcasecmp(Name, "HideMenu")) ConfigChannelBlocker.hidemenu = atoi(Value);
    if (!strcasecmp(Name, "FilterList")) ConfigChannelBlocker.filterlist = atoi(Value);
    return true;
}

bool cPluginChannelBlocker::Service(const char *Id, void *Data)
{
  return false;
}

const char **cPluginChannelBlocker::SVDRPHelpPages(void)
{
  return NULL;
}

cString cPluginChannelBlocker::SVDRPCommand(const char *Command, const char *Option, int &ReplyCode)
{
  return NULL;
}

VDRPLUGINCREATOR(cPluginChannelBlocker); // Don't touch this!
