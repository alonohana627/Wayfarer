#include "story.hpp"

#include "items.hpp"
#include "monsters.hpp"

#include <array>
#include <map>
#include <string>
#include <vector>

namespace wayfarer::game {

namespace {

// helpers

using gamelib::core::DialogueChoice;
using gamelib::core::DialogueLink;
using gamelib::core::DialogueNode;
using gamelib::core::DialogueTree;
using gamelib::core::Objective;
using gamelib::core::ObjectiveKind;
using gamelib::core::QuestDef;

constexpr int speciesSubject(const SpeciesId s) { return static_cast<int>(s); }
constexpr int npc(const NpcId n) { return static_cast<int>(n); }
constexpr int boss(const BossId b) { return kBossSubject + static_cast<int>(b); }

std::vector<QuestDef> buildQuests()
{
    using N = NpcId;
    using S = SpeciesId;
    std::vector<QuestDef> q;
    // The main story.
    q.push_back({.id = kQuestArrival, .giver = npc(N::Elder), .turnInTo = npc(N::Dorran),
                 .objectives = {{ObjectiveKind::Talk, npc(N::Dorran), 1}}, .reward = {.experience = 30, .coins = 50}});
    q.push_back({.id = kQuestChapter1, .giver = npc(N::Elder), .requiredQuests = {kQuestArrival},
                 .objectives = {{ObjectiveKind::Defeat, boss(BossId::Grandcap), 1}},
                 .reward = {.experience = 500, .coins = 400, .items = {{kOrangePotion, 5}}}});
    q.push_back({.id = kQuestChapter2, .giver = npc(N::Brann), .minLevel = 6, .requiredQuests = {kQuestChapter1},
                 .objectives = {{ObjectiveKind::Defeat, boss(BossId::Rumblerock), 1}},
                 .reward = {.experience = 1800, .coins = 1200, .items = {{kOrangePotion, 10}}}});
    q.push_back({.id = kQuestChapter3, .giver = npc(N::Pippa), .minLevel = 12, .requiredQuests = {kQuestChapter2},
                 .objectives = {{ObjectiveKind::Defeat, boss(BossId::Murkshell), 1}},
                 .reward = {.experience = 4200, .coins = 2600, .items = {{kWhitePotion, 5}}}});
    q.push_back({.id = kQuestChapter4, .giver = npc(N::Lumi), .minLevel = 18, .requiredQuests = {kQuestChapter3},
                 .objectives = {{ObjectiveKind::Defeat, boss(BossId::Warden), 1}},
                 .reward = {.experience = 8000, .coins = 4500, .items = {{kWhitePotion, 10}}}});
    q.push_back({.id = kQuestChapter5, .giver = npc(N::Elder), .minLevel = 24, .requiredQuests = {kQuestChapter4},
                 .objectives = {{ObjectiveKind::Defeat, boss(BossId::Hush), 1}}, .reward = {.experience = 12000, .coins = 8000}});
    // Side quests.
    q.push_back({.id = 11, .giver = npc(N::Dorran), .requiredQuests = {kQuestArrival},
                 .objectives = {{ObjectiveKind::Defeat, speciesSubject(S::Bellcap), 10}},
                 .reward = {.experience = 90, .coins = 120, .items = {{kRedPotion, 8}}}});
    q.push_back({.id = 12, .giver = npc(N::Pippa), .minLevel = 2, .requiredQuests = {kQuestArrival},
                 .objectives = {{ObjectiveKind::Collect, materialItem(Material::JellyDrop), 6}},
                 .reward = {.experience = 110, .coins = 160, .items = {{kBluePotion, 8}}}});
    q.push_back({.id = kQuestLetter, .giver = npc(N::Elder), .turnInTo = npc(N::Brann), .minLevel = 3,
                 .requiredQuests = {kQuestArrival}, .objectives = {{ObjectiveKind::Deliver, npc(N::Brann), 1}},
                 .reward = {.experience = 140, .coins = 250, .items = {{kReturnScroll, 3}}}});
    q.push_back({.id = 14, .giver = npc(N::Nell), .minLevel = 3, .requiredQuests = {kQuestArrival},
                 .objectives = {{ObjectiveKind::Collect, materialItem(Material::Acorn), 5}},
                 .reward = {.experience = 160, .coins = 100, .items = {{kOrangePotion, 4}}}});
    q.push_back({.id = 15, .giver = npc(N::Brann), .minLevel = 8, .requiredQuests = {kQuestChapter1},
                 .objectives = {{ObjectiveKind::Collect, materialItem(Material::GlimmerOre), 8}},
                 .reward = {.experience = 700, .coins = 600, .items = {{equipItem(2, Slot::Weapon, 0), 1}}}});
    q.push_back({.id = 16, .giver = npc(N::Dorran), .minLevel = 9, .requiredQuests = {kQuestChapter1},
                 .objectives = {{ObjectiveKind::Defeat, speciesSubject(S::Flitter), 12}},
                 .reward = {.experience = 900, .coins = 700, .items = {{kManaTonic, 5}}}});
    q.push_back({.id = kQuestStarCharm, .giver = npc(N::Lumi), .minLevel = 13, .requiredQuests = {kQuestChapter2},
                 .objectives = {{ObjectiveKind::Collect, kStarCharm, 1}},
                 .reward = {.experience = 1800, .coins = 1000, .items = {{equipItem(3, Slot::Ring, 1), 1}}}});
    q.push_back({.id = 18, .giver = npc(N::Pippa), .minLevel = 14, .requiredQuests = {kQuestChapter2},
                 .objectives = {{ObjectiveKind::Collect, materialItem(Material::SwirlShell), 8}},
                 .reward = {.experience = 2200, .coins = 1500, .items = {{kWhitePotion, 6}}}});
    q.push_back({.id = 19, .giver = npc(N::Lumi), .minLevel = 19, .requiredQuests = {kQuestChapter3},
                 .objectives = {{ObjectiveKind::Collect, materialItem(Material::WispLight), 10}},
                 .reward = {.experience = 4000, .coins = 2400, .items = {{kManaTonic, 10}}}});
    q.push_back({.id = 20, .giver = npc(N::Nell), .minLevel = 24, .requiredQuests = {kQuestChapter4},
                 .objectives = {{ObjectiveKind::Collect, materialItem(Material::FrostFur), 10}},
                 .reward = {.experience = 7000, .coins = 4000, .items = {{kWhitePotion, 15}}}});
    return q;
}

const std::vector<QuestDef>& quests()
{
    static const std::vector<QuestDef> q = buildQuests();
    return q;
}

const std::map<int, QuestText>& texts()
{
    static const std::map<int, QuestText> t{
        {kQuestArrival, {kQuestArrival, "A Wayfarer Arrives", "Find Captain Dorran at the west gate."}},
        {kQuestChapter1, {kQuestChapter1, "The Restless Elder", "Calm Grandcap in the glade beyond the Rootway."}},
        {kQuestChapter2, {kQuestChapter2, "Rumble in the Deep", "Quiet Rumblerock at the bottom of the Glimmerdeep."}},
        {kQuestChapter3, {kQuestChapter3, "Queen of the Mire", "Soothe Queen Murkshell in her pool."}},
        {kQuestChapter4, {kQuestChapter4, "The Silent Lanterns", "Rekindle the Lantern Warden in Sunspire."}},
        {kQuestChapter5, {kQuestChapter5, "The Hush", "Climb Frostcrown Peak and face the Hush."}},
        {11, {11, "Bellcap Trouble", "Defeat 10 Bellcaps on the Bellwood Path."}},
        {12, {12, "Sticky Supplies", "Bring Pippa 6 Jelly Drops from the Pink Mochis."}},
        {kQuestLetter, {kQuestLetter, "A Letter for the Forge", "Deliver the Elder's letter to Brann."}},
        {14, {14, "Nell's Acorn Pie", "Gather 5 Acorns from the Pudgies."}},
        {15, {15, "Glimmer for the Forge", "Bring Brann 8 Glimmer Ore from the mines."}},
        {16, {16, "Wings in the Dark", "Defeat 12 Flitters in the Glimmerdeep."}},
        {kQuestStarCharm, {kQuestStarCharm, "Lumi's Star Charm", "Find the Star Charm a Bog Mochi swallowed."}},
        {18, {18, "Shells for the Shop", "Bring Pippa 8 Swirl Shells from the marsh."}},
        {19, {19, "Wisp Lights", "Collect 10 Wisp Lights for Lumi's telescope."}},
        {20, {20, "Warm for Winter", "Gather 10 Frost Fur for Nell's mittens."}},
    };
    return t;
}

// One quest's lines at its giver (and at the NPC it is handed in to).
struct Offer {
    int quest = 0;
    std::string offer;    // the giver asks
    std::string accepted; // after "I'll help"
    std::string active;   // while it is open
};

struct Handin {
    int quest = 0;
    std::string done; // handed in
};

struct Chat {
    std::string condition;
    std::string text;
};

class TreeBuilder {
public:
    void npcTree(const NpcId who, const std::vector<Offer>& gives, const std::vector<Handin>& takes,
                 const std::vector<Chat>& chats, const std::vector<DialogueChoice>& services)
    {
        const std::string root = npcRoot(who);
        const std::string speaker{npcName(who)};
        DialogueNode rootNode{.id = root};
        // Hand-ins first, then offers, then reminders, then small talk.
        for (const Handin& q : takes) {
            const std::string id = root + "_done" + std::to_string(q.quest);
            rootNode.next.push_back({.target = id, .condition = "complete " + std::to_string(q.quest)});
            m_tree.add({.id = id, .speaker = speaker, .text = q.done, .next = {{.target = root + "_menu"}},
                        .effects = {"turnin " + std::to_string(q.quest)}});
        }
        for (const Offer& q : gives) {
            const std::string id = root + "_offer" + std::to_string(q.quest);
            rootNode.next.push_back({.target = id, .condition = "available " + std::to_string(q.quest)});
            m_tree.add({.id = id, .speaker = speaker, .text = q.offer,
                        .choices = {{.label = "I'll help!", .target = id + "_yes", .effects = {"accept " + std::to_string(q.quest)}},
                                    {.label = "Maybe later.", .target = ""}}});
            m_tree.add({.id = id + "_yes", .speaker = speaker, .text = q.accepted});
        }
        for (const Offer& q : gives) {
            if (q.active.empty()) {
                continue;
            }
            const std::string id = root + "_active" + std::to_string(q.quest);
            rootNode.next.push_back({.target = id, .condition = "active " + std::to_string(q.quest)});
            m_tree.add({.id = id, .speaker = speaker, .text = q.active, .next = {{.target = root + "_menu"}}});
        }
        for (std::size_t i = 0; i < chats.size(); ++i) {
            const std::string id = root + "_chat" + std::to_string(i);
            rootNode.next.push_back({.target = id, .condition = chats[i].condition});
            m_tree.add({.id = id, .speaker = speaker, .text = chats[i].text, .next = {{.target = root + "_menu"}}});
        }
        m_tree.add(std::move(rootNode));
        // After talking: the NPC's services, or the end.
        if (services.empty()) {
            m_tree.add({.id = root + "_menu"});
        } else {
            std::vector<DialogueChoice> choices = services;
            choices.push_back({.label = "Goodbye!", .target = ""});
            m_tree.add({.id = root + "_menu", .speaker = speaker, .text = "Anything else?", .choices = choices});
        }
    }

    DialogueTree take() { return std::move(m_tree); }
    void add(DialogueNode node) { m_tree.add(std::move(node)); }

private:
    DialogueTree m_tree;
};

DialogueTree buildTree()
{
    TreeBuilder t;
    using N = NpcId;

    // Elder Oswin: the story's voice.
    t.npcTree(N::Elder,
              {{kQuestArrival,
                "Ah, a wayfarer! Welcome to Hearthvale. You've come at a strange time... a hush has fallen over the "
                "lands, and our five waystones have gone dim. Would you lend us your sword?",
                "Wonderful! First, find Captain Dorran at the west gate. He'll show you how we get around here.", ""},
               {kQuestChapter1,
                "The forest waystone glows again, thanks to you. But old Grandcap, the gentle elder of Bellwood, has "
                "turned restless. Past the Rootway lies his glade. Will you calm him?",
                "Take the forest waystone in the plaza. Be kind to him if you can... and careful.",
                "Grandcap waits beyond the Rootway, at the far end of Mossy Hollow."},
               {kQuestLetter,
                "Would you take this letter to Brann at the forge? My knees and his stairs are no longer friends.",
                "Thank you, dear. Brann's forge is the one with the smoking chimney.", "Brann's forge is east of the plaza."},
               {kQuestChapter5,
                "Four guardians calmed. Only the peak remains... and the Hush itself. It took our songs and our "
                "colors. Climb Frostcrown Peak, wayfarer, and bring them home.",
                "The last waystone is yours. Whatever you find up there, Hearthvale is with you.",
                "The Hush waits at the Crown of Frost, above the ice caves."}},
              {},
              {{"chapter 6", "Listen... birdsong, bells, laughter. You brought it all back. Stay as long as you like."},
               {"night", "The stars are out. Lumi will be up on her hill, I expect. Get some rest at the inn, too."},
               {"chapter 1", "The forest is just beyond the green waystone. Mind the bellcaps; they bounce."},
               {"chapter 2", "Brann has been grumbling about the mines. Go and see him at the forge."},
               {"chapter 3", "Pippa's supplies come up from the marsh. Something has stopped them."},
               {"chapter 4", "Lumi has been watching the ruins at night. Talk to her after dark."},
               {"", "Tea? No? Ah well. The kettle's always on at the inn."}},
              {});

    // Captain Dorran: the guard who teaches the hyper jump.
    t.npcTree(N::Dorran,
              {{11, "Bellcaps have been bouncing all over the path. Clear out ten of them and I'll owe you one.",
                "Good. Remember: jump, and jump again in the air!", "Ten bellcaps. They bob just outside town."},
               {16, "Flitters swarm the mine tunnels. Twelve of them would make Brann's miners very happy.",
                "Swing early. They swoop.", "Twelve flitters in the Glimmerdeep."}},
              {{kQuestArrival, "So the Elder sent you? Then watch closely. Jump, and while you're in the air, jump again: a Hyper "
                "Jump! Hold up to fly straight up instead. You'll get the hang of it."},
               {11, "Ten bellcaps! Here, potions for the road. You're a natural."},
               {16, "Not a single flitter left buzzing. Brilliant work, wayfarer."}},
              {{"chapter 6", "Peace and quiet... the good kind. I might even take a day off."},
               {"night", "Night watch. The lamps keep the plaza cozy, but the roads are dark."},
               {"", "Stay sharp out there. Arrow keys to move, X to strike, Z to jump."}},
              {});

    // Pippa: the general store.
    t.npcTree(N::Pippa,
              {{12, "Ooh, a new face! Could you bring me six Jelly Drops? The pink mochis drop them. I make the best "
                    "potions with them!",
                "Yay! Six jelly drops, wobbly and fresh!", "Six Jelly Drops, please! The pink mochis have them."},
               {kQuestChapter3,
                "My marsh deliveries stopped coming! The carters say a giant shell-queen has taken the mire. Could you... "
                "um... ask her nicely? With your sword?",
                "You're the best! The marsh waystone is open now.", "Queen Murkshell lives in the deepest pool of the marsh."},
               {18, "Swirl shells make the prettiest buttons. Eight of them, please?",
                "Thank you! Shiny shells, shiny buttons!", "Eight Swirl Shells from the turbanshells."}},
              {{12, "Perfect, they're still wobbly! Here, take some of my blue potions."},
               {kQuestChapter3, "The carts are rolling again! Here, my finest potions, on the house."},
               {18, "Look how they shimmer! You've earned this."}},
              {{"chapter 6", "Business is booming! Everyone wants festival ribbons."},
               {"", "Welcome to Pippa's! Potions, scrolls, and a little bit of everything."}},
              {{.label = "Let me see your wares.", .target = "", .effects = {"shop"}}});

    // Brann: the blacksmith.
    t.npcTree(N::Brann,
              {{kQuestChapter2,
                "Hm. You're the one who calmed Grandcap? Then hear this: the mines have gone quiet, and something big "
                "rumbles at the bottom. No ore, no steel. Go down and see.",
                "The mine waystone's lit. Take a lantern. Hm, you have one? Good.",
                "Rumblerock. Bottom of the Glimmerdeep. Can't miss the noise."},
               {15, "Bring me eight Glimmer Ore and I'll forge you something proper.",
                "Hm. Good.", "Eight Glimmer Ore. The pebblets carry it."}},
              {{kQuestLetter, "A letter from Oswin? Hm... he wants me to look after you. Fine. Here, scrolls to get home quick."},
               {kQuestChapter2, "Hear that? Picks ringing again. Hm. You did good, wayfarer."},
               {15, "Fine ore. Fine work. Take this saber; it's yours."}},
              {{"chapter 6", "Hm. Even my anvil sounds happier."},
               {"", "Hm. Bring me ore and coin, I'll make your gear sing."}},
              {{.label = "Upgrade my gear.", .target = "", .effects = {"smith"}}});

    // Lumi: the stargazer, out at night.
    t.npcTree(N::Lumi,
              {{kQuestStarCharm,
                "Oh! You startled me. I lost my star charm in the marsh... I think a bog mochi swallowed it. Could you "
                "find it?",
                "Thank you! It twinkles, so you'll know it.", "A bog mochi in the marsh has my star charm."},
               {kQuestChapter4,
                "The lanterns of Sunspire went dark the night the Hush came. Their warden still walks the old court... "
                "lost. Will you rekindle it?",
                "The ruins' waystone is open. Follow the light.", "The Lantern Warden is in the court above the vault."},
               {19, "Wisp lights make my telescope see further. Ten, please?", "Ten little lights!",
                "Ten Wisp Lights from the ruins."}},
              {{kQuestStarCharm, "My charm! It's warm from the mochi. Here, a ring for you."},
               {kQuestChapter4, "The ruins are glowing again... I can see them from here! Thank you."},
               {19, "Oh, I can see the moon's freckles now! Thank you!"}},
              {{"chapter 6", "Look! A shooting star. Make a wish... I already made mine, and it came true."},
               {"", "Shh... the stars are talking. That one is called the Wayfarer, you know."}},
              {});

    // Nell: the baker's kid, out by day.
    t.npcTree(N::Nell,
              {{14, "Hi hi! I'm baking an acorn pie! Can you get me five acorns? The pudgies hoard them!",
                "Five acorns! Don't eat them on the way!", "Five acorns! The pudgies in Mossy Hollow have them."},
               {20, "Brrr, winter's coming. Ten frost furs would make mittens for the whole town!", "Mittens for everyone!",
                "Ten Frost Furs from the peak."}},
              {{14, "Acorns! The pie will be ready... later. Here, take these potions now!"},
               {20, "Fluffy! Everyone will have warm hands. You're the best!"}},
              {{"chapter 6", "There's going to be a festival! With pies! LOTS of pies!"},
               {"", "Mom says I can't have pie before lunch. It's always before lunch."}},
              {});

    // The inn's door: rest.
    t.add({.id = "inn", .speaker = "The Sleepy Kettle Inn",
           .text = "A warm bed and a cup of tea. Your journey is written down whenever you return to town.",
           .choices = {{.label = "Rest until morning.", .target = "", .effects = {"sleep morning"}},
                       {.label = "Rest until night.", .target = "", .effects = {"sleep night"}},
                       {.label = "Not now.", .target = ""}}});
    return t.take();
}

} // namespace

std::string_view npcName(const NpcId n)
{
    switch (n) {
    case NpcId::Elder: return "Elder Oswin";
    case NpcId::Pippa: return "Pippa";
    case NpcId::Brann: return "Brann";
    case NpcId::Lumi: return "Lumi";
    case NpcId::Dorran: return "Captain Dorran";
    case NpcId::Nell: return "Nell";
    case NpcId::Count: break;
    }
    return "";
}

std::string_view npcRole(const NpcId n)
{
    switch (n) {
    case NpcId::Elder: return "town elder";
    case NpcId::Pippa: return "general store";
    case NpcId::Brann: return "blacksmith";
    case NpcId::Lumi: return "stargazer";
    case NpcId::Dorran: return "town guard";
    case NpcId::Nell: return "baker's kid";
    case NpcId::Count: break;
    }
    return "";
}

std::string npcRoot(const NpcId n)
{
    return "npc" + std::to_string(static_cast<int>(n));
}

std::span<const QuestDef> questDefs()
{
    return quests();
}

const QuestText& questText(const int id)
{
    static const QuestText none{};
    const auto it = texts().find(id);
    return it == texts().end() ? none : it->second;
}

const DialogueTree& dialogueTree()
{
    static const DialogueTree tree = buildTree();
    return tree;
}

int chapterQuestFor(const BossId b)
{
    return kQuestChapter1 + static_cast<int>(b);
}

} // namespace wayfarer::game
