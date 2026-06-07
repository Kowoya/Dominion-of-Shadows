#include <iostream>
#include <string>
#include <vector>
#include <memory>
#include <type_traits>
#include <limits>
#include <clocale>

using std::cout;
using std::endl;
using std::string;
using std::vector;
using std::unique_ptr;
using std::make_unique;

enum class Alignment { Light, Shadow, Neutral };
enum class QuestStatus { NotStarted, InProgress, Completed };
enum class QuestType { Main, Side };
enum class PlayerPath { Undecided, Light, Shadow, Balance };

// ---------------------- Helper structs -------------------

struct DialogueLine {
    string speaker;
    string text;
};

class Dialogue {
    vector<DialogueLine> lines;
public:
    void addLine(const string& speaker, const string& text) {
        lines.push_back({ speaker, text });
    }

    void play() const {
        for (const auto& l : lines) {
            cout << l.speaker << ": " << l.text << "\n";
        }
        cout << "----------------------------------------\n";
    }

    bool empty() const { return lines.empty(); }
};

class Scene {
    string id;
    string title;
    string description;
    Dialogue dialogue;
public:
    Scene(const string& id, const string& title, const string& desc)
        : id(id), title(title), description(desc) {}

    void setDialogue(const Dialogue& d) { dialogue = d; }

    void playIntro() const {
        cout << "\n=== СЦЕНА: " << title << " ===\n";
        cout << description << "\n\n";
    }

    void playFull() const {
        playIntro();
        if (!dialogue.empty()) {
            dialogue.play();
        }
    }

    const string& getId() const { return id; }
};

// ========================== ENTITY / CHARACTERS ========================

class Entity {
protected:
    string name;
    int health;

public:
    Entity(const string& n, int h)
        : name(n), health(h) {}

    virtual ~Entity() = default;

    virtual void takeDamage(int amount) {
        health -= amount;
        if (health < 0) health = 0;
    }

    bool isAlive() const { return health > 0; }

    const string& getName() const { return name; }

    virtual void update() = 0;
};

class Character : public Entity {
protected:
    int corruption;
    Alignment alignment;

public:
    Character(const string& n, int h, Alignment a)
        : Entity(n, h), corruption(0), alignment(a) {}

    void changeCorruption(int delta) {
        corruption += delta;
        if (corruption < 0) corruption = 0;
    }

    int getCorruption() const { return corruption; }
    Alignment getAlignment() const { return alignment; }

    virtual void speak() const = 0;
};

class Ritual;
class Item;

// ========================== PLAYER ========================

class Player : public Character {
    vector<unique_ptr<Item>> inventory;
    vector<unique_ptr<Ritual>> knownRituals;
    PlayerPath path;
    int level;
    int xp;

    int xpToNextLevel() const {
        // Проста формула: що вищий рівень, то більше потрібно XP
        return 100 * level;
    }

public:
    Player(const string& n)
        : Character(n, 100, Alignment::Neutral),
        path(PlayerPath::Undecided),
        level(1),
        xp(0) {}

    void learnRitual(unique_ptr<Ritual> r);
    void castRitual(size_t index, Entity& target);

    void choosePath(PlayerPath newPath) { path = newPath; }
    PlayerPath getPath() const { return path; }

    void addItem(unique_ptr<Item> item) {
        inventory.push_back(std::move(item));
    }

    void speak() const override {
        cout << name << ": Я новобранець Дев’ятого Дому.\n";
    }

    void update() override {
        if (alignment == Alignment::Shadow) {
            changeCorruption(1);
        }
    }

    void addXP(int amount) {
        xp += amount;
        cout << name << " отримує " << amount << " досвіду.\n";
        // Перевіряємо підвищення рівня
        while (xp >= xpToNextLevel()) {
            xp -= xpToNextLevel();
            level++;
            cout << "\n=== РІВЕНЬ ПІДВИЩЕНО! ===\n";
            cout << name << " досягає " << level << " рівня!\n\n";
        }
    }

    int getLevel() const { return level; }
    int getXP() const { return xp; }
};

// ========================== NPC ========================

class NPC : public Character {
protected:
    int trustLevel;
public:
    NPC(const string& n, int h, Alignment a)
        : Character(n, h, a), trustLevel(0) {}

    void changeTrust(int delta) { trustLevel += delta; }
    int getTrust() const { return trustLevel; }

    virtual void interact(Player& player) = 0;
    virtual void describeBackstory() const = 0;
};

class Ritual;

// Наставник Корвіан
class Mentor : public NPC {
public:
    Mentor(const string& n)
        : NPC(n, 80, Alignment::Neutral) {}

    void interact(Player& player) override;
    void speak() const override {
        cout << name << ": Дев’ятий Дім небезпечний… але необхідний.\n";
    }

    void update() override {}

    void describeBackstory() const override {
        cout << "[Корвіан]: Колись моя родина загинула через неконтрольований портал.\n"
            << "Відтоді я не дозволяю світові гинути… навіть якщо для цього доводиться жертвувати людьми.\n\n";
    }
};

// Рейвен з Кривавого Дому
class Rival : public NPC {
public:
    Rival(const string& n)
        : NPC(n, 90, Alignment::Shadow) {}

    void interact(Player& player) override {
        cout << name << ": Я доведу, що Кривавий Дім не слабший за Дев’ятий.\n";
    }

    void speak() const override {
        cout << name << ": Ми проливаємо кров не заради задоволення, а заради результату.\n";
    }

    void update() override {}

    void describeBackstory() const override {
        cout << "[Рейвен]: Я вбив свою першу людину в чотирнадцять.\n"
            << "Це був ритуал. Відтоді шукаю спосіб проливати менше крові.\n\n";
    }
};

// Ліра — студентка
class Librarian : public NPC {
public:
    Librarian(const string& n)
        : NPC(n, 50, Alignment::Light) {}

    void interact(Player& player) override {
        cout << name << ": Я чую шепіт між книжками… Ти теж його чуєш?\n";
    }

    void speak() const override {
        cout << name << ": Якщо книги починають шепотіти — це вже поганий знак.\n";
    }

    void update() override {}

    void describeBackstory() const override {
        cout << "[Ліра]: Я просто хотіла вчитись.\n"
            << "Я відкрила не ту книгу… і тепер бачу те, чого не повинна.\n\n";
    }
};

// ========================== DEMON ========================

class Demon : public Entity {
protected:
    int power;
public:
    Demon(const string& n, int h, int p)
        : Entity(n, h), power(p) {}

    virtual void tempt(Player& player) = 0;
    virtual void roar() const = 0;
};

class Valkar : public Demon {
public:
    Valkar()
        : Demon("Валкар Істинолама", 150, 50) {}

    void tempt(Player& player) override {
        cout << name << ": Віддай мені частину себе — і я дам тобі силу та істину.\n";
        player.changeCorruption(15);
    }

    void roar() const override {
        cout << name << " виходить з розлому, його голос ламає тишу старого театру.\n";
    }

    void update() override {
        if (health < 150) health++;
    }
};

// ========================== HOUSE ========================

class House {
protected:
    string name;
    int reputation;

public:
    House(const string& n)
        : name(n), reputation(0) {}

    virtual ~House() = default;

    void changeReputation(int delta) { reputation += delta; }
    int getReputation() const { return reputation; }

    virtual void applyBonus(Player& player) = 0;
    virtual void describe() const = 0;
};

class NinthHouse : public House {
public:
    NinthHouse() : House("Дев’ятий Дім") {}

    void applyBonus(Player& player) override {
        cout << "Дев’ятий Дім посилює твої темні ритуали.\n";
    }

    void describe() const override {
        cout << "[Дев’ятий Дім]: Дім, що вивчає найтемніші ритуали, щоб контролювати хаос.\n";
    }
};

class BloodHouse : public House {
public:
    BloodHouse() : House("Кривавий Дім") {}

    void applyBonus(Player& player) override {
        cout << "Кривавий Дім дає більше сили за рахунок здоров’я.\n";
    }

    void describe() const override {
        cout << "[Кривавий Дім]: Вони вірять, що кров — найчесніша валюта.\n";
    }
};

// ========================== RITUALS ========================

class Ritual {
protected:
    string name;
    int healthCost;
    int corruptionGain;

public:
    Ritual(const string& n, int hc, int cg)
        : name(n), healthCost(hc), corruptionGain(cg) {}

    virtual ~Ritual() = default;

    virtual void perform(Player& caster, Entity& target) = 0;

    virtual string description() const {
        return name + " (hp cost=" + std::to_string(healthCost) + ")";
    }
};

class ProtectiveRitual : public Ritual {
public:
    ProtectiveRitual(const string& n, int hc, int cg)
        : Ritual(n, hc, cg) {}

    void perform(Player& caster, Entity& target) override {
        cout << caster.getName() << " створює захисний бар’єр навколо " << target.getName() << ".\n";
        caster.changeCorruption(corruptionGain);
    }
};

class SummoningRitual : public Ritual {
public:
    SummoningRitual(const string& n, int hc, int cg)
        : Ritual(n, hc, cg) {}

    void perform(Player& caster, Entity& target) override {
        cout << caster.getName() << " викликає тінь, що атакує " << target.getName() << "!\n";
        target.takeDamage(25);
        caster.changeCorruption(corruptionGain);
    }
};

// ========================== ITEMS ========================

class Item {
protected:
    string name;
    int rarity;
public:
    Item(const string& n, int r)
        : name(n), rarity(r) {}

    virtual ~Item() = default;

    virtual void use(Player& player) = 0;

    const string& getName() const { return name; }
};

class Potion : public Item {
    int healAmount;
public:
    Potion(const string& n, int r, int heal)
        : Item(n, r), healAmount(heal) {}

    void use(Player& player) override {
        cout << "Гравець випиває " << name << " і почувається краще (логіку лікування можна додати).\n";
    }
};

// Реалізація Player-методів
void Player::learnRitual(unique_ptr<Ritual> r) {
    cout << name << " вивчає ритуал: " << r->description() << "\n";
    knownRituals.push_back(std::move(r));
}

void Player::castRitual(size_t index, Entity& target) {
    if (index >= knownRituals.size()) {
        cout << "Невідомий ритуал.\n";
        return;
    }
    knownRituals[index]->perform(*this, target);
}

// ========================== СТАТИЧНИЙ ПОЛІМОРФІЗМ ========================

template<typename TRitual, typename TTarget>
void safeCastRitual(TRitual& ritual, Player& caster, TTarget& target) {
    static_assert(std::is_base_of<Ritual, TRitual>::value,
        "TRitual must derive from Ritual");
    if (target.isAlive()) {
        ritual.perform(caster, target);
    }
}

// ========================== QUEST SYSTEM ========================

class QuestObjective {
    string description;
    bool completed;
public:
    QuestObjective(const string& desc)
        : description(desc), completed(false) {}

    void complete() { completed = true; }
    bool isCompleted() const { return completed; }
    const string& getDescription() const { return description; }
};

class Quest {
protected:
    string id;
    string title;
    string description;
    QuestStatus status;
    QuestType type;
    vector<QuestObjective> objectives;

public:
    Quest(const string& id, const string& title, const string& desc, QuestType type)
        : id(id), title(title), description(desc), status(QuestStatus::NotStarted), type(type) {}

    virtual ~Quest() = default;

    virtual void start(Player& player) {
        status = QuestStatus::InProgress;
        cout << "\n[КВЕСТ ПОЧАТОК] " << title << "\n" << description << "\n";
    }

    virtual void progress(Player& player) {
        cout << "Поточні цілі:\n";
        for (const auto& obj : objectives) {
            cout << " - " << obj.getDescription()
                << (obj.isCompleted() ? " [Виконано]\n" : " [Невиконано]\n");
        }
    }

    virtual void complete(Player& player) {
        status = QuestStatus::Completed;
        cout << "[КВЕСТ ЗАВЕРШЕНО] " << title << "\n";
    }

    void addObjective(const QuestObjective& obj) {
        objectives.push_back(obj);
    }

    void completeObjective(size_t index) {
        if (index < objectives.size()) {
            objectives[index].complete();
        }
    }

    QuestStatus getStatus() const { return status; }
    QuestType getType() const { return type; }
    const string& getId() const { return id; }
};

class MainQuest : public Quest {
public:
    MainQuest(const string& id, const string& title, const string& desc)
        : Quest(id, title, desc, QuestType::Main) {}
};

class SideQuest : public Quest {
public:
    SideQuest(const string& id, const string& title, const string& desc)
        : Quest(id, title, desc, QuestType::Side) {}
};

// ========================== GAME ========================

class Game {
    Player player;
    Mentor corvian;
    Rival raven;
    Librarian lira;
    Valkar valkar;
    NinthHouse ninth;
    BloodHouse blood;

    vector<Scene> scenes;
    vector<unique_ptr<Quest>> quests;

    bool prologueDone = false;
    bool demonDone = false;

public:
    Game()
        : player("Новобранець"),
        corvian("Корвіан"),
        raven("Рейвен"),
        lira("Ліра"),
        valkar(),
        ninth(),
        blood()
    {
        initScenes();
        initQuests();
    }

    int askChoice(const vector<string>& options, const string& prompt = "Обери варіант:") {
        while (true) {
            cout << "\n" << prompt << "\n";
            for (size_t i = 0; i < options.size(); ++i) {
                cout << "  " << (i + 1) << ") " << options[i] << "\n";
            }
            cout << "> ";
            int choice;
            if (!(std::cin >> choice)) {
                std::cin.clear();
                std::cin.ignore(std::numeric_limits<std::streamsize>::max(), '\n');
                cout << "Введи номер варіанту.\n";
                continue;
            }
            if (choice >= 1 && choice <= (int)options.size()) {
                std::cin.ignore(std::numeric_limits<std::streamsize>::max(), '\n');
                return choice;
            }
            cout << "Неправильний вибір, спробуй ще.\n";
        }
    }

    // Нова версія — з перевіркою рівня
    int askChoiceWithLevel(const vector<string>& options,
        const vector<int>& requiredLevels,
        const string& prompt = "Обери варіант:") {
        if (options.size() != requiredLevels.size()) {
            // fallback, щоб не зламати гру
            return askChoice(options, prompt);
        }

        while (true) {
            cout << "\n" << prompt << "\n";
            for (size_t i = 0; i < options.size(); ++i) {
                cout << "  " << (i + 1) << ") " << options[i];
                if (player.getLevel() < requiredLevels[i]) {
                    cout << " [потрібен рівень " << requiredLevels[i] << "]";
                }
                cout << "\n";
            }
            cout << "> ";
            int choice;
            if (!(std::cin >> choice)) {
                std::cin.clear();
                std::cin.ignore(std::numeric_limits<std::streamsize>::max(), '\n');
                cout << "Введи номер варіанту.\n";
                continue;
            }
            if (choice >= 1 && choice <= (int)options.size()) {
                size_t idx = (size_t)(choice - 1);
                if (player.getLevel() < requiredLevels[idx]) {
                    cout << "Твій рівень занизький для цієї відповіді. Потрібен рівень "
                        << requiredLevels[idx] << ".\n";
                    continue;
                }
                std::cin.ignore(std::numeric_limits<std::streamsize>::max(), '\n');
                return choice;
            }
            cout << "Неправильний вибір, спробуй ще.\n";
        }
    }

    void waitContinue() {
        cout << "\nНатисни Enter, щоб продовжити...";
        std::cin.ignore(std::numeric_limits<std::streamsize>::max(), '\n');
    }

    void initScenes() {
        // 0 — СОН У РУЇНАХ МАЙБУТНЬОГО
        scenes.emplace_back(
            "dream",
            "Сон у руїнах майбутнього",
            "Небо над Еліором розірване, мов тканина під ножем. Крізь чорні провали сочиться "
            "фіолетове світло, яке пульсує у такт твоєму серцю. Повітря густе — кожен вдих "
            "відчувається, ніби диханням туману зі склепу.\n\n"
            "Вулицями рухаються спотворені фігури. Вони не мають облич, але повертають голови, "
            "коли ти проходиш повз, і з порожніх очниць лунає приглушений плач. Каміння тріскається "
            "під ногами — місто буквально стікає тінню.\n\n"
            "У центрі площі стоїть величезне око, вп’ятеро більше собору. Його зіниця пульсує, "
            "як жива істота. Коли ти дивишся в нього, воно дивиться глибше — прямо в тебе.\n\n"
            "Темрява знижується… і світ розпадається, мов попіл."
        );

        // 1 — ШЛЯХ УНИЗ
        scenes.emplace_back(
            "stairs",
            "Шлях униз",
            "Сходи розбиті, слизькі від вологи й часу. Ліхтар у твоїй руці здригається від "
            "кожного подуву невидимого вітру. Камінь стін теплий на дотик — наче тут досі живе "
            "щось велике, щось спляче.\n\n"
            "Символи Домів вирізьблені глибокими борознами — деякі ще вкриті засохлою кров’ю. "
            "У глибині коридору іноді чути шепіт, але варто обернутись — і там нікого.\n\n"
            "Двері Дев’ятого Дому високі, чорні, вкриті металевими написами мовою, якої ти не "
            "знаєш, але розумієш."
        );

        // 2 — РИТУАЛ ІНІЦІАЦІЇ
        scenes.emplace_back(
            "initiation",
            "Ритуал ініціації",
            "Зала схожа на перевернуту каплицю. Свічки ростуть зі стін, мов кісткові нарости. "
            "Їхнє полум’я не коливається, але тягнеться до тебе.\n\n"
            "Кров’яне коло — ще тепле. На камені в центрі пульсує тріщина, і з неї просочується "
            "чорний дим із запахом металу й вогкості.\n\n"
            "Мітка на твоїй руці рухається. Вона стискається, мов жива істота, і відповідає болем "
            "кожному слову Корвіана.\n\n"
            "Коли ти стаєш у коло — приміщення дихає разом із тобою."
        );

        // 3 — ПРОВУЛОК МЕРТВИХ
        scenes.emplace_back(
            "alley",
            "Провулок мертвих",
            "Світло ліхтарів відбивається у калюжах, змішаних із кров’ю. Повітря важке від гнилі "
            "й страху.\n\n"
            "Тіло лежить неприродно — ніби хтось зламав його, перш ніж убити. Очниці темні, з них "
            "стікає чорна рідина, що повільно випаровується.\n\n"
            "Поруч на стіні — символ. Він вигравіруваний ніби зсередини, наче сама цегла хотіла його "
            "народити.\n\n"
            "Коли ти дивишся на тіло занадто довго — починаєш чути шепіт."
        );

        // 4 — КРИВАВИЙ СОБОР
        scenes.emplace_back(
            "blood_cathedral",
            "Кривавий собор",
            "Собор давно закинутий, але пахне свіжою кров’ю. Підлога слизька. Колони тріщать, ніби "
            "гнуться під чужою вагою.\n\n"
            "Світло проходить крізь вітражі, забарвлюючи все у червоно-багряний — неначе саме світло "
            "стікає кров’ю.\n\n"
            "Рейвен стоїть спокійно… занадто спокійно.\n\n"
            "Його очі — холодні, мов клинки."
        );

        // 5 — РОЗЛОМ НА ПЛОЩІ
        scenes.emplace_back(
            "square_rift",
            "Розлом на площі",
            "Площа — у хаосі. Люди кричать, збивають одне одного, тікають. Фонтан зруйнований, "
            "вода виливається чорними струмками.\n\n"
            "В центрі — розлом. Він схожий на рвану рану в самій реальності. Краї мерехтять синім та "
            "фіолетовим, ніби живі.\n\n"
            "З тіней виповзають істоти з ламаними тілами.\n\n"
            "Ти бачиш — це тепер твоя війна."
        );

        // 6 — ТЕАТР ТІНІ
        scenes.emplace_back(
            "theatre",
            "Театр Тіні",
            "Тут колись сміялись.\nТепер — шепочуть.\n\n"
            "Крісла прогнили. На сцені — розірвані ляльки, уламки люстр і старі афіші, "
            "з яких стерті обличчя акторів.\n\n"
            "Розлом схожий на зіницю, що розплющилась у центрі сцени. Він дивиться на тебе.\n\n"
            "З нього неспішно виходить Валкар. Його форма пливе, ніби ртуть."
        );

        // 7 — ТАЄМНИЙ АРХІВ
        scenes.emplace_back(
            "archive",
            "Таємний архів",
            "Приміщення пахне воском, гниллю та давніми молитвами.\n\n"
            "Книги — не просто книги. Їхні обкладинки теплі, ніби шкіра. Деякі тихо стогнуть, коли ти "
            "береш їх до рук.\n\n"
            "У записах —\nімена,\nдати,\nжертви.\n\n"
            "Місто використовували, як лабораторних щурів."
        );

        // 8 — ФІНАЛ СВІТЛА
        scenes.emplace_back(
            "final_light",
            "Фінал Світла",
            "Палаюче небо. Закат не червоний — він золотий, але болючий для очей.\n\n"
            "Коли ти піднімаєш ритуальний символ — він випалює руку до кістки… але ти не відпускаєш.\n\n"
            "Азраель палає, мов жива пожежа."
        );

        // 9 — ФІНАЛ ТІНІ
        scenes.emplace_back(
            "final_shadow",
            "Фінал Тіні",
            "Темний трон із кісток. Демони мовчки схилили голови.\n\n"
            "Корвіан занадто старий. Занадто слабкий.\n\n"
            "Він плаче."
        );

        // 10 — ФІНАЛ БАЛАНСУ
        scenes.emplace_back(
            "final_balance",
            "Фінал Балансу",
            "Білий простір пульсує, мов серце.\n\n"
            "Тінь – твоє відображення.\n\n"
            "Вона посміхається… твоєю посмішкою."
        );

        // 11 — ЕПІЛОГ
        scenes.emplace_back(
            "epilog",
            "Епілог",
            "Місто не таке, як раніше."
        );

        // 12 — Тіні в гуртожитку
        scenes.emplace_back(
            "dorm_shadows",
            "Тіні в гуртожитку",
            "Коридор гуртожитку потонув у напівтемряві. Лампи то гаснуть, то спалахують знову, "
            "відкидаючи на стіни довгі, неприродні тіні.\n\n"
            "За одними дверима хтось плаче. За іншими — чути сміх, якого не повинно бути о цій годині."
        );

        // 13 — Морг. Очі, що пам’ятають
        scenes.emplace_back(
            "morgue",
            "Морг: Очі, що пам’ятають",
            "Морг холодний, але піт стікає тобі по спині. Металеві столи блищать, наче лід. "
            "Простирадла, під якими — тіла, нагадують рядок безмовних знаків.\n\n"
            "Один із мішків ледь помітно рухається, хоча ти знаєш — всередині вже давно немає життя."
        );

        // 14 — Чужі голоси (запас під майбутній квест)
        scenes.emplace_back(
            "voices",
            "Чужі голоси",
            "Ти стоїш у темній аудиторії. Вікна зачинені, але повітря повне шепоту. "
            "Голоси перекривають одне одного, наче ти підслуховуєш десятки розмов одразу.\n\n"
            "І раптом один із голосів вимовляє твоє ім’я… і точно переказує твій дитячий страх."
        );
    }

    void initQuests() {
        auto q1 = make_unique<MainQuest>("M1", "Печатка під університетом",
            "Тебе викликають на 'додатковий іспит' і ведуть у заборонене підземелля.");
        q1->addObjective(QuestObjective("Спуститися в підземелля"));
        q1->addObjective(QuestObjective("Зустрітися з Корвіаном"));
        q1->addObjective(QuestObjective("Пройти ритуал ініціації"));
        quests.push_back(std::move(q1));

        auto q2 = make_unique<MainQuest>("M3", "Мертві без очей",
            "У місті з’являються тіла без очей. Треба розслідувати ці вбивства.");
        q2->addObjective(QuestObjective("Оглянути місце злочину у провулку"));
        q2->addObjective(QuestObjective("Знайти зв’язок із Домами"));
        quests.push_back(std::move(q2));

        auto q3 = make_unique<SideQuest>("S1", "Загублені конспекти",
            "Студентка Ліра втратила конспекти і чує шепоти в бібліотеці.");
        q3->addObjective(QuestObjective("Поговорити з Лірою в бібліотеці"));
        q3->addObjective(QuestObjective("Знайти конспекти"));
        q3->addObjective(QuestObjective("Вирішити долю духа, що їх украв"));
        quests.push_back(std::move(q3));

        auto q4 = make_unique<SideQuest>("S2", "Тіні в гуртожитку",
            "Студенти скаржаться на дивні звуки та поведінку сусіда, який, здається, розмовляє з кимось у порожній кімнаті.");
        q4->addObjective(QuestObjective("Оглянути гуртожиток уночі"));
        q4->addObjective(QuestObjective("Поговорити з одержимим студентом"));
        q4->addObjective(QuestObjective("Вирішити: врятувати його чи знищити тінь разом із ним"));
        quests.push_back(std::move(q4));

        auto q5 = make_unique<SideQuest>("S3", "Очі, що пам’ятають",
            "Одна з жертв ритуальних вбивств шепоче твоє ім’я з моргу. Вона щось знає про демона.");
        q5->addObjective(QuestObjective("Спуститися до моргу"));
        q5->addObjective(QuestObjective("Провести ритуал спілкування з мертвими"));
        q5->addObjective(QuestObjective("Отримати попередження від душі"));
        quests.push_back(std::move(q5));
    }

    Quest* findQuest(const string& id) {
        for (auto& q : quests) {
            if (q->getId() == id) return q.get();
        }
        return nullptr;
    }

    bool isQuestCompleted(const string& id) {
        Quest* q = findQuest(id);
        return q && q->getStatus() == QuestStatus::Completed;
    }

    bool isQuestNotCompleted(const string& id) {
        Quest* q = findQuest(id);
        return !q || q->getStatus() != QuestStatus::Completed;
    }

    // ---------- ПРОЛОГ ----------

    void prologue() {
        scenes[0].playIntro();
        cout << "Голос Тіні: Ти не перший… ти не останній… але ти — ключ.\n";
        cout << "Інший голос (ледь чутно): Або — чужий ніж у чужому серці…\n";
        prologueDone = true;
        waitContinue();
    }

    // ---------- АКТ I: ініціація (M1) ----------

    void mainQuestInitiation() {
        Quest* q1 = findQuest("M1");
        if (!q1 || q1->getStatus() == QuestStatus::Completed) {
            cout << "Цей квест уже завершено.\n";
            return;
        }

        q1->start(player);

        // ЦІЛЬ 0: Спуститися в підземелля
        scenes[1].playIntro();
        q1->completeObjective(0);

        // ЦІЛЬ 1: Зустрітися з Корвіаном
        scenes[2].playIntro();
        q1->completeObjective(1);

        corvian.describeBackstory();

        cout << "Корвіан: Ти запізнився… але, можливо, саме вчасно.\n";
        int c1 = askChoice({
            "«Хто ви і що це за місце?»",
            "«Це якийсь жарт?»",
            "«Я знаю, що тут не просто університет.»"
            });

        if (c1 == 1) {
            cout << "Корвіан: Це межа між тим, що ти вважаєш реальністю… і тим, що ховається під нею.\n";
            corvian.changeTrust(+1);
        }
        else if (c1 == 2) {
            cout << "Корвіан: Ні. І скоро тобі буде не до сміху.\n";
            corvian.changeTrust(-1);
        }
        else {
            cout << "Корвіан: О, ти кращий за більшість. Але знати — ще не означає бути готовим.\n";
            corvian.changeTrust(+2);
        }

        int c2 = askChoice({
            "«Я піду до кінця.»",
            "«У мене взагалі є вибір?»",
            "«Якщо я відмовлюсь — що стане зі мною?»"
            }, "Корвіан: Якщо ти зробиш крок у коло — дороги назад не буде.");

        if (c2 == 1) {
            cout << "Корвіан: Темрява любить хоробрих… або дурних.\n";
        }
        else if (c2 == 2) {
            cout << "Корвіан: Усі думають, що мають вибір. А потім прокидаються на іншому боці.\n";
        }
        else {
            cout << "Корвіан: Якщо відмовишся — ти забудеш усе це… але світ не забуде про тебе.\n";
        }

        cout << "\nТи стаєш у кров’яне коло. Мітка на руці палає, зал ніби дихає разом із тобою.\n";
        int c3 = askChoice({
            "«Я хочу сили.»",
            "«Я хочу правду.»",
            "«Я хочу змін.»"
            }, "Корвіан: Скажи вголос — чого ти хочеш насправді?");

        if (c3 == 1) {
            cout << "Ти відчуваєш, як щось темне відгукується всередині.\n";
            player.changeCorruption(5);
            player.choosePath(PlayerPath::Shadow);
        }
        else if (c3 == 2) {
            cout << "Світ неначе стає різкішим, але холоднішим.\n";
            player.choosePath(PlayerPath::Light);
        }
        else {
            cout << "Ти відчуваєш, як тінь і світло стикаються всередині тебе.\n";
            player.choosePath(PlayerPath::Balance);
        }

        corvian.interact(player);

        // ЦІЛЬ 2: Пройти ритуал ініціації
        q1->completeObjective(2);

        q1->progress(player);
        cout << "Ти вижив. Мітка Тіні запечатана на твоїй руці.\n";

        // XP за основний квест
        player.addXP(100);

        q1->complete(player);

        waitContinue();
    }

    // ---------- SIDE QUEST: ЛІРА (S1) ----------

    void sideQuestLira() {
        Quest* q3 = findQuest("S1");
        if (!q3) return;
        if (!isQuestCompleted("M1")) {
            cout << "Спершу потрібно пройти ініціацію в Дев’ятому Домі.\n";
            return;
        }
        if (q3->getStatus() == QuestStatus::Completed) {
            cout << "Цей побічний квест уже завершено.\n";
            return;
        }

        cout << "\n--- Побічний квест: Ліра та загублені конспекти ---\n";

        q3->start(player);
        lira.describeBackstory();

        cout << "Ти знаходиш Ліру в бібліотеці. Вона тремтить і тримає порожню теку.\n";
        lira.interact(player);
        cout << "Ліра: Вони говорять крізь сторінки… іноді навіть через людей.\n";

        // ЦІЛЬ 0: поговорити з Лірою
        q3->completeObjective(0);

        int c = askChoice({
            "«Я допоможу тобі знайти конспекти.»",
            "«Це, напевно, просто втома. Відпочинь.»",
            "«Ти чула голос, що називає моє ім’я?»"
            });

        if (c == 1) {
            cout << "Ліра: Дякую… Мені страшно, але з тобою трохи спокійніше.\n";
            lira.changeTrust(+2);
        }
        else if (c == 2) {
            cout << "Ліра: Може й так… Але чому тоді шепіт не стихає?\n";
            lira.changeTrust(-1);
        }
        else {
            cout << "Ліра: Так. Він сміявся, але не моїм голосом.\n";
            lira.changeTrust(+1);
        }

        cout << "\nТи досліджуєш її кімнату і знаходиш дрібного духа-жартівника, "
            "що ховається під ліжком і тримає конспекти.\n";
        cout << "Дух: «Вона завжди читала вголос. Я лише слухав…»\n";

        // ЦІЛЬ 1: знайти конспекти
        q3->completeObjective(1);

        int c2 = askChoice({
            "Вигнати духа ритуалом (менше корупції, Ліра в безпеці).",
            "Залишити духа як потенційного союзника (більше корупції, але потенційний бонус)."
            }, "Що ти зробиш?");

        if (c2 == 1) {
            cout << "Ти виганяєш духа. Кімната стає тихішою, шепіт слабшає.\n";
            player.changeCorruption(2);
        }
        else {
            cout << "Ти укладаєш мовчазну угоду з духом. Він зникає в тінях, але ти відчуваєш його погляд.\n";
            player.changeCorruption(6);
        }

        // ЦІЛЬ 2: вирішити долю духа
        q3->completeObjective(2);

        q3->progress(player);

        // XP за побічний квест
        player.addXP(70);

        q3->complete(player);
        waitContinue();
    }

    // ---------- SIDE QUEST: ТІНІ В ГУРТОЖИТКУ (S2) ----------

    void sideQuestDormShadows() {
        Quest* q = findQuest("S2");
        if (!q) return;
        if (!isQuestCompleted("M1")) {
            cout << "Тобі потрібно спочатку пройти ініціацію, щоб мати право втручатися в життя студентів.\n";
            return;
        }
        if (q->getStatus() == QuestStatus::Completed) {
            cout << "Цей побічний квест уже завершено.\n";
            return;
        }

        q->start(player);
        scenes[12].playIntro();

        // ЦІЛЬ 0: оглянути гуртожиток
        q->completeObjective(0);

        cout << "Студентка у піжамі: Ти теж це чув? Він розмовляє сам із собою… уже третю ніч.\n";
        cout << "Інший голос із сусідньої кімнати шепоче: «Він не сам… він НІКОЛИ не сам.»\n";

        cout << "Ти підходиш до дверей кімнати, звідки лунає шепіт.\n";
        int c1 = askChoice({
            "Постукати й чемно покликати студента.",
            "Відчинити двері ритуалом.",
            "Прислухатись до шепоту, перш ніж увійти."
            });

        if (c1 == 1) {
            cout << "Голос студента: «Зайнято… Я… не один…»\n";
        }
        else if (c1 == 2) {
            cout << "Замок клацає, ніби з полегшенням. Тінь біля стелі відступає глибше в кут.\n";
            player.changeCorruption(3);
        }
        else {
            cout << "Шепіт: «Не входь… якщо боїшся побачити себе з іншого боку дверей.»\n";
        }

        cout << "\nВсередині ти бачиш студента з чорними колами під очима. Він сидить на ліжку й дивиться у кут.\n";
        cout << "Студент: «Ви його теж бачите? Він каже, що може зробити мене сильним…»\n";

        // ЦІЛЬ 1: поговорити з одержимим
        q->completeObjective(1);

        int c2 = askChoiceWithLevel(
            {
                "«Це не сила. Це паразит. Я можу тебе звільнити.»",
                "«Якщо приймеш його — ти станеш небезпечним. І для себе, і для інших.»",
                "«Скажи йому, що я прийшов. Хай вийде.»"
            },
            { 1, 1, 2 }, // третя репліка тільки з 2 рівня
            "Що ти скажеш студенту?"
        );

        if (c2 == 1) {
            cout << "Студент: «Ти правда зможеш?.. Добре. Я… я довірюсь тобі.»\n";
        }
        else if (c2 == 2) {
            cout << "Студент: «Небезпечним?.. Може, це єдиний спосіб вижити в цьому місті.»\n";
        }
        else {
            cout << "Повітря холоне. Шепіт стає голоснішим: «Ти забрав у мене одну іграшку… не забереш другу.»\n";
            player.changeCorruption(4);
        }

        cout << "\nТи готуєшся до ритуалу вигнання.\n";
        int c3 = askChoice({
            "Вигнати тінь, зберігши життя студента (складніший ритуал, більше ризику).",
            "Розірвати зв’язок радикально — знищити й тінь, і носія."
            }, "Як ти завершиш цей квест?");

        if (c3 == 1) {
            cout << "Ти створюєш у повітрі знаки Дев’ятого Дому. Тінь виривається з тіла студента, "
                "кричить багатьма голосами й тане.\n";
            cout << "Студент падає, але дихає.\n";
            player.changeCorruption(5);
        }
        else {
            cout << "Ти спрямовуєш ритуал прямо в серце зв’язку. Тінь і тіло зливаються в один крик… "
                "а потім на ліжку лишається лише порожня оболонка.\n";
            cout << "Коридор гуртожитку на кілька секунд стає абсолютно тихим.\n";
            player.changeCorruption(10);
        }

        // ЦІЛЬ 2: вирішити долю
        q->completeObjective(2);

        q->progress(player);

        // XP за побічний квест
        player.addXP(80);

        q->complete(player);
        waitContinue();
    }

    // ---------- SIDE QUEST: ОЧІ, ЩО ПАМ’ЯТАЮТЬ (S3) ----------

    void sideQuestEyesRemember() {
        Quest* q = findQuest("S3");
        if (!q) return;
        if (!isQuestCompleted("M3")) {
            cout << "Спершу потрібно розслідувати вбивства (квест М3), щоб знати, кому ставити питання.\n";
            return;
        }
        if (q->getStatus() == QuestStatus::Completed) {
            cout << "Цей побічний квест уже завершено.\n";
            return;
        }

        q->start(player);
        scenes[13].playIntro();

        // ЦІЛЬ 0: спуститися до моргу
        q->completeObjective(0);

        cout << "Черговий у морзі: «Ти знову з Домів… У мене від вас мороз по шкірі. "
            "Скажи хоч, що цього разу буде тихо.»\n";

        int c1 = askChoice({
            "«Якщо пощастить — душа просто відповість і засне.»",
            "«Я нічого не обіцяю.»"
            });

        if (c1 == 1) {
            cout << "Черговий нервово киває: «Ладно… Я буду в коридорі. Якщо щось піде не так — "
                "я просто зроблю вигляд, що цього не бачив.»\n";
        }
        else {
            cout << "Черговий зітхає: «Звісно… Як завжди.» Він виходить, тихо закриваючи за собою двері.\n";
        }

        cout << "\nТи підходиш до одного з мішків. Шепіт ледь чутний, але ти впізнаєш своє ім’я.\n";
        cout << "Голос з мішка: «Ти… запізнився…»\n";

        // ЦІЛЬ 1: провести ритуал
        q->completeObjective(1);

        int c2 = askChoiceWithLevel(
            {
                "«Хто це зробив з тобою?»",
                "«Чому ти кличеш саме мене?»",
                "«Що ти бачила ПЕРЕД смертю?»"
            },
            { 1, 1, 3 }, // третє питання вимагає 3 рівень
            "Що ти запитаєш у душі?"
        );

        if (c2 == 1) {
            cout << "Голос: «Він не був людиною… Він був порожнечею в плоті. "
                "Його очі були, як тріщина в небі…»\n";
        }
        else if (c2 == 2) {
            cout << "Голос: «Ти… носиш ту саму мітку… ти стоїш там, де стояли вони… до всього цього…»\n";
        }
        else {
            cout << "Голос: «Я бачила… як він торкався стіни… і символ сам вирізався в цеглі. "
                "Це був не ніж… це була воля.»\n";
        }

        cout << "\nТи починаєш ритуал спілкування глибшого рівня.\n";

        int c3 = askChoice({
            "Заспокоїти душу й дати їй піти (менше інформації, менше корупції).",
            "Притиснути її питаннями, змушуючи згадувати (більше інформації, більше корупції)."
            }, "Як ти продовжиш?");

        if (c3 == 1) {
            cout << "Ти промовляєш формули заспокоєння. Голос стає тихішим: "
                "«Дякую… Хоч хтось… дослухав…»\n";
            player.changeCorruption(3);
        }
        else {
            cout << "Душа кричить: «Він… він уже поруч з тобою… Він шепоче в тіні кожного Дому… "
                "Валкар — не єдина відповідь…»\n";
            player.changeCorruption(8);
            cout << "Перед очима блимає образ: Корвіан стоїть перед тим самим символом, що й на стінах провулку.\n";
        }

        // ЦІЛЬ 2: отримати попередження
        q->completeObjective(2);

        q->progress(player);

        // XP за побічний квест
        player.addXP(90);

        q->complete(player);
        waitContinue();
    }

    // ---------- АКТ II: РИТУАЛЬНІ ВБИВСТВА (M3) ----------

    void mainQuestMurders() {
        Quest* q2 = findQuest("M3");
        if (!q2) return;
        if (!isQuestCompleted("M1")) {
            cout << "Спочатку ти маєш пройти ініціацію (квест M1).\n";
            return;
        }
        if (q2->getStatus() == QuestStatus::Completed) {
            cout << "Цей квест уже завершено.\n";
            return;
        }

        cout << "\n=== Основний квест: Мертві без очей ===\n";
        q2->start(player);

        // Сцена 3: провулок мертвих
        scenes[3].playIntro();
        cout << "Капітан варти: «Ми знайшли вже третього… І всі — без очей. Це щось з ваших, так?»\n";
        cout << "Гравець: «Це не ритуал, який схвалив би хоч один Дім.»\n";
        cout << "Капітан варти: «Мене не хвилює, хто це схвалив. Мене хвилює, хто це зупинить.»\n";

        // ЦІЛЬ 0: оглянути місце злочину
        q2->completeObjective(0);

        q2->progress(player);
        waitContinue();

        // Сцена 4: кривавий собор — зустріч з Рейвеном
        scenes[4].playIntro();
        raven.describeBackstory();
        cout << "Рейвен: «Дев’ятий Дім знову сунеться туди, де ллється кров.»\n";

        int c = askChoiceWithLevel(
            {
                "«Це твої ритуали?»",
                "«Мені байдуже, з якого ти Дому. Людей убивають.»",
                "«Якщо це не ви — допоможи мені знайти того, хто це робить.»"
            },
            { 1, 1, 2 }, // третій варіант – більш "лідерський"
            "Що ти скажеш Рейвену?"
        );

        if (c == 1) {
            cout << "Рейвен: «Наші ритуали криваві, але не боягузливі. Ми не вириваємо очі мертвим.»\n";
            raven.changeTrust(+1);
        }
        else if (c == 2) {
            cout << "Рейвен: «Говориш правильно. Може, з тебе ще буде толк.»\n";
            raven.changeTrust(+2);
        }
        else {
            cout << "Рейвен: «Ти просиш Кривавий Дім про союз?.. Сміливо. І небезпечно.»\n";
            raven.changeTrust(+3);
        }

        cout << "Ти розумієш, що хтось краде ритуали різних Домів і перекручує їх.\n";

        // ЦІЛЬ 1: знайти зв'язок із Домами
        q2->completeObjective(1);

        q2->progress(player);

        // XP за основний квест
        player.addXP(120);

        q2->complete(player);

        waitContinue();

        // Сцена 5: розлом на площі (атмосферна)
        scenes[5].playIntro();
        cout << "Капітан варти: «Якщо ти не зупиниш це — то хто?!»\n";
        waitContinue();
    }

    // ---------- АКТ III: ДЕМОН ----------

    void mainQuestDemon() {
        if (!isQuestCompleted("M3")) {
            cout << "Спершу потрібно розслідувати вбивства (квест M3).\n";
            return;
        }
        if (demonDone) {
            cout << "Зустріч із Валкаром уже відбулась.\n";
            return;
        }

        cout << "\n=== Основний квест: Театр Тіні (Валкар) ===\n";

        // Сцена 6: театр
        scenes[6].playIntro();
        valkar.roar();

        cout << "Валкар: «Тут колись говорили слова, що нічого не значили. "
            "Тепер тут буде сказано щось справжнє.»\n";
        cout << "Гравець: «Ти демон з розлому?»\n";
        cout << "Валкар: «Я — наслідок ваших бажань. Ви просили сили. Ви просили захисту. "
            "Ви просили правди. Ось я.»\n\n";

        int c = askChoiceWithLevel(
            {
                "«Я знищу тебе.»",
                "«Чого ти хочеш?»",
                "«Ти вже був у моїх снах… чи не так?»"
            },
            { 1, 1, 3 }, // фраза про сни — тільки з 3 рівня
            "Як ти відповіси Валкару?"
        );

        if (c == 1) {
            cout << "Валкар сміється: «Всі так кажуть. А потім підписують контракт власною кров’ю.»\n";
        }
        else if (c == 2) {
            cout << "Валкар: «Я хочу свободи. І ти можеш стати моїм ключем.»\n";
        }
        else {
            cout << "Валкар: «Сни — це репетиція реальності. Ти непогано тримаєшся для актора, "
                "який не знав про свою роль.»\n";
            player.changeCorruption(2);
        }

        int c2 = askChoice({
            "Прийняти угоду Валкара (сила + корупція).",
            "Відмовитись і спробувати боротись."
            }, "Валкар простягає тобі руку з тіні.");

        if (c2 == 1) {
            cout << "Ти відчуваєш, як частина тебе відривається і тоне у розломі.\n";
            valkar.tempt(player);
        }
        else {
            cout << "Ти відкидаєш його руку. Повітря стискається — попереду бій.\n";
            ProtectiveRitual shield("Щит Тіні", 10, 2);
            cout << "\nТи наспіх твориш захисний ритуал.\n";
            safeCastRitual(shield, player, valkar);
        }

        cout << "\nКорупція гравця зараз: " << player.getCorruption() << "\n";
        waitContinue();

        // Сцена 7: таємний архів
        cout << "\nТрохи згодом ти опиняєшся в таємному архіві Дев’ятого Дому.\n";
        scenes[7].playIntro();
        cout << "Запис Корвіана: «Якщо світ — човен, то люди — баласт. "
            "Іноді, щоб урятувати судно, треба щось викинути за борт.»\n";
        waitContinue();

        // XP за акт з демоном
        player.addXP(150);

        demonDone = true;
    }

    // ---------- ФІНАЛИ ----------

    void finalAct() {
        if (!demonDone || !isQuestCompleted("M3")) {
            cout << "Ти ще не готовий до фіналу — спочатку заверши основні квести.\n";
            return;
        }

        cout << "\n=== АКТ IV: ФІНАЛИ ===\n";

        PlayerPath path = player.getPath();
        int corr = player.getCorruption();

        if (path == PlayerPath::Shadow || corr >= 25) {
            scenes[9].playIntro();
            cout << "Корвіан: «Ти був моєю надією… а став моїм вироком.»\n";
        }
        else if (path == PlayerPath::Light && corr <= 12) {
            scenes[8].playIntro();
            cout << "Азраель: «Ти врятуєш людство… але загубиш себе.»\n";
        }
        else {
            scenes[10].playIntro();
            cout << "Тінь: «Знищ мене — і ти станеш ніким. Прийми — і станеш усім.»\n";
        }

        waitContinue();

        scenes[11].playIntro();
        cout << "Голос автора: «Світ не поділяється на світло і темряву. Він стоїть між ними.»\n";
    }

    void runDemo() {
        cout << "=== DOMINION OF SHADOWS — КОНСОЛЬНА DEMO-ІСТОРІЯ ===\n";
        cout << "Ти — новобранець Дев’ятого Дому в місті Еліор.\n\n";

        cout << "=== Світ: Доми ===\n";
        ninth.describe();
        blood.describe();
        cout << "\n";

        bool running = true;
        while (running) {
            vector<string> options;
            vector<int> actions;

            if (!prologueDone) {
                options.push_back("Пролог: Сон у руїнах майбутнього");
                actions.push_back(1);
            }
            if (isQuestNotCompleted("M1")) {
                options.push_back("Основний квест: Печатка під університетом (ініціація)");
                actions.push_back(2);
            }
            if (isQuestCompleted("M1") && isQuestNotCompleted("S1")) {
                options.push_back("Побічний квест: Ліра та загублені конспекти");
                actions.push_back(3);
            }
            if (isQuestCompleted("M1") && isQuestNotCompleted("S2")) {
                options.push_back("Побічний квест: Тіні в гуртожитку");
                actions.push_back(8);
            }
            if (isQuestCompleted("M1") && isQuestNotCompleted("M3")) {
                options.push_back("Основний квест: Мертві без очей (ритуальні вбивства)");
                actions.push_back(4);
            }
            if (isQuestCompleted("M3") && isQuestNotCompleted("S3")) {
                options.push_back("Побічний квест: Очі, що пам’ятають (морг)");
                actions.push_back(9);
            }
            if (isQuestCompleted("M3") && !demonDone) {
                options.push_back("Основний квест: Театр Тіні (зустріч із Валкаром)");
                actions.push_back(5);
            }
            if (isQuestCompleted("M3") && demonDone) {
                options.push_back("Перейти до фіналу");
                actions.push_back(6);
            }

            options.push_back("Показати поточний стан персонажа");
            actions.push_back(7);
            options.push_back("Вийти з гри");
            actions.push_back(0);

            int idx = askChoice(options, "Оберіть дію:");
            int action = actions[idx - 1];

            switch (action) {
            case 1: prologue(); break;
            case 2: mainQuestInitiation(); break;
            case 3: sideQuestLira(); break;
            case 4: mainQuestMurders(); break;
            case 5: mainQuestDemon(); break;
            case 6:
                finalAct();
                running = false;
                break;
            case 7:
                cout << "\n=== СТАН ПЕРСОНАЖА ===\n";
                cout << "Рівень: " << player.getLevel() << "\n";
                cout << "Поточний XP: " << player.getXP() << "\n";
                cout << "Корупція: " << player.getCorruption() << "\n";
                cout << "Шлях: ";
                switch (player.getPath()) {
                case PlayerPath::Undecided: cout << "ще не обрано\n"; break;
                case PlayerPath::Light: cout << "Світло\n"; break;
                case PlayerPath::Shadow: cout << "Тінь\n"; break;
                case PlayerPath::Balance: cout << "Баланс\n"; break;
                }
                break;
            case 8: sideQuestDormShadows(); break;
            case 9: sideQuestEyesRemember(); break;
            case 0:
                running = false;
                break;
            }
        }

        cout << "\n=== Кінець демо-кампанії ===\n";
        cout << "Твій фінальний рівень: " << player.getLevel() << "\n";
        cout << "Фінальна корупція: " << player.getCorruption() << "\n";
    }
};

// Реалізація Mentor::interact після оголошення Game
void Mentor::interact(Player& player) {
    cout << name << ": Я навчу тебе захисному ритуалу.\n";
    auto ritual = make_unique<ProtectiveRitual>("Щит Тіні", 10, 2);
    player.learnRitual(std::move(ritual));
}

// ========================== main ========================

int main() {
    std::setlocale(LC_ALL, "");

    Game game;
    game.runDemo();

    return 0;
}
