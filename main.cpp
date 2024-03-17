#include <bits/stdc++.h>
#include <fstream>
#include <vector>
#include <cstdio>
#include <iostream>
#include <queue>
#include <mutex>

using namespace std;
// x-> downwards
// y-> rightwards

#define x_plus 3
#define x_minus 2
#define y_plus 0
#define y_minus 1

#define to_good 0
#define to_berth 1
#define free 2

const int n = 200;
const int robot_num = 10;
const int berth_num = 10;
const int N = 210;

int money, boat_capacity, id;
char ch[N][N];
int gds[N][N];
int last_frame = 0;
int current_frame = 0;
int delta_frame = 0;
const int max_ttl = 1000;

unique_ptr<fstream> log_p;

vector<pair<int, int>> findPath(pair<int, int> start, pair<int, int> end);
inline int enCode(pair<int, int> cur, pair<int, int> next);
struct Berth // 泊位
{
    int x;
    int y;
    int transport_time;
    int loading_speed;
    Berth() {}
    Berth(int x, int y, int transport_time, int loading_speed)
    {
        this->x = x;
        this->y = y;
        this->transport_time = transport_time;
        this->loading_speed = loading_speed;
    }
} berth[berth_num + 10];

struct Boat // 船
{
    int num, pos, status;
} boat[10];

class Good
{
public:
    int x, y, val, birth;
    Good(int _x, int _y, int _val, int _ber) : x(_x), y(_y), val(_val), birth(_ber){};
};
vector<Good> Goods;

struct CompareByDistance
{
    bool operator()(const pair<int, int> &a, const pair<int, int> &b)
    {
        return a.first > b.first;
    }
};

struct Robot
{
    int x, y, goods, id;
    int status=0;
    // int mbx, mby;
    vector<int> codes;
    int step = 0;
    int max_step = 0;
    int state = free;
    Robot() {}
    Robot(int startX, int startY)
    {
        x = startX;
        y = startY;
        codes.resize(100);
    }

    int select_good()
    // return good id
    // assign codes and max_step
    // set step to 0

    {
        step = 0;
        if (Goods.empty())
        {
            return -1;
        }
        priority_queue<pair<int, int>, vector<pair<int, int>>, CompareByDistance> pq;
        // pq.first is distance to good and second is good id;
        for (int i = 0; i < Goods.size(); i++)
        {
            pq.push({abs(Goods[i].x - x) + abs(Goods[i].y - y), i});
        }
        bool needed = true;
        while (needed && !pq.empty())
        {
            auto good = pq.top();
            pq.pop();
            auto len = findPath({x, y}, {Goods[good.second].x, Goods[good.second].y});
            *log_p << "******robot id: " << id << "******" << endl;
            *log_p << "len.size() is:" << len.size() << endl;
            *log_p << "dest is" << Goods[good.second].x << " " << Goods[good.second].y << endl;
            if (len.size() + current_frame < max_ttl + Goods[good.second].birth)
            {
                needed = false;
                state = to_good;
                max_step = len.size() - 1;
                codes.resize(max_step);
                *log_p << "(" << len.back().first << "," << len.back().second << ")";
                for (int i = max_step; i > 0; i--)
                {
                    codes[max_step - i] = enCode(len[i], len[i - 1]);
                    *log_p << "=" << codes[max_step - i] << ">(" << len[i - 1].first << "," << len[i - 1].second << ")";
                }
                *log_p << endl;
                Goods.erase(Goods.begin() + good.second);
                return 1;
            }
        }
        return -1;
    }

    void poll(int id)
    {
        fstream f("./Demo/log" + to_string(id) + ".txt", std::ios_base::in | std::ios_base::out | std::ios_base::app);
        f << "**********frame: " << current_frame << "**********" << endl;
        f << "x: " << x << " y: " << y << "payload from judeg:" << ((goods == 1) ? "has" : "not") <<"status from judge: "<<((status==1)?"normal":"recover")<<endl;;
        if (state == free)
        {
            f<<"state: free" << endl;
            int good_id = select_good();
            if (good_id != -1)
            {
                *log_p << "robot: " << id << "goods.size is:" << Goods.size() << endl;
                step = 0;
                printf("move %d %d\n", id, codes[step]);
                f << "move " << id << " " << codes[step] << endl;
                fflush(stdout);
                step++;
                state = to_good;
            }
            else
            {
                *log_p << "robot: " << id << "goods.size is:" << Goods.size() << endl;
                state = free;
            }
        }
        else if (state == to_good)
        {
            f << " state: to_good" << endl;
            printf("move %d %d\n", id, codes[step]);
            f << "move " << id << " " << codes[step] << endl;
            fflush(stdout);
            step++;
            if (step == max_step - 1)
            {
                printf("get %d\n", id);
                f << "get " << id << endl;
                fflush(stdout);
                state = to_berth;
                auto len = findPath({x, y}, {berth[0].x, berth[0].y});
                max_step = len.size();
                step = 0;
            }
        }
        else if (state == to_berth)
        {
            f << "state: to_berth" << endl;
            printf("move %d %d\n", id, codes[step]);
            f << "move " << id << " " << codes[step] << endl;
            fflush(stdout);
            step++;
            if (step == max_step - 1)
            {
                printf("pull %d\n", id);
                f << "pull " << id << endl;
                fflush(stdout);
                state = free;
            }
        }
    }

} robot[robot_num + 10];

#define field_char '.'
#define sea_char '*'
#define block_char '#'
#define robot_char 'A'
#define berth_char 'B'

struct Node
{
    int x, y;     // 节点坐标
    int g, h;     // 起点到当前节点的代价和启发式代价
    Node *parent; // 父节点

    Node(int x, int y, int g, int h, Node *parent) : x(x), y(y), g(g), h(h), parent(parent) {}

    int f() const
    {
        return g + h;
    }
};

struct CompareNode
{
    bool operator()(Node *a, Node *b)
    {
        return a->f() > b->f();
    }
};

vector<pair<int, int>> findPath(pair<int, int> start, pair<int, int> end)
// just return path
{
    vector<pair<int, int>> directions = {{-1, 0}, {1, 0}, {0, -1}, {0, 1}}; // 上下左右四个方向

    // 创建优先队列 openList 存放待处理的节点
    priority_queue<Node *, vector<Node *>, CompareNode> openList;
    // 创建二维布尔数组 closed 标记已处理的节点
    vector<vector<bool>> closed(200, vector<bool>(200, false));

    Node *startNode = new Node(start.first, start.second, 0, abs(end.first - start.first) + abs(end.second - start.second), nullptr);
    openList.push(startNode);

    while (!openList.empty())
    {
        Node *current = openList.top();
        openList.pop();

        if (current->x == end.first && current->y == end.second)
        {
            // 找到路径，回溯父节点得到路径
            vector<pair<int, int>> path;
            while (current != nullptr)
            {
                path.push_back({current->x, current->y});
                current = current->parent;
            }
            return path;
        }

        closed[current->x][current->y] = true;

        char ch[200][200]; // Declare the variable 'ch' before using it

        for (auto &dir : directions)
        {
            // 计算相邻节点在 x 和 y 方向上的坐标位置
            int newX = current->x + dir.first;
            int newY = current->y + dir.second;

            if (newX > 0 && newX <= 200 && newY > 0 && newY <= 200&& !closed[newX][newY] && ch[newX][newY] != sea_char && ch[newX][newY] != block_char)
            {
                int g = current->g + 1;
                int h = abs(end.first - newX) + abs(end.second - newY);
                Node *neighbor = new Node(newX, newY, g, h, current);
                openList.push(neighbor);
            }
        }
    }

    return {}; // 没有找到路径
}

void Init()
{
    std::fstream file("../Demo/log.txt", std::ios_base::in | std::ios_base::out | std::ios_base::app);
    file << "Init" << std::endl;
    cerr << "Init" << endl;
    for (int i = 1; i <=n; i++)
        scanf("%s", ch[i]+1);
    for (int i = 0; i < berth_num; i++)
    {
        int id;
        scanf("%d", &id);
        scanf("%d%d%d%d", &berth[id].x, &berth[id].y, &berth[id].transport_time, &berth[id].loading_speed);
        file << "berth" << id << " x: " << berth[id].x << " y: " << berth[id].y << " transport_time: " << berth[id].transport_time << " loading_speed: " << berth[id].loading_speed << endl;
    }
    scanf("%d", &boat_capacity);
    char okk[100];
    scanf("%s", okk);
    printf("OK\n");
    file << okk << std::endl;
    file << "begin to log" << endl;
    file.close();
    fflush(stdout);
}

int Input()
{
    scanf("%d%d", &id, &money);
    int num;
    scanf("%d", &num);
    for (int i = 1; i <= num; i++)
    {
        int x, y, val;
        scanf("%d%d%d", &x, &y, &val);
        Goods.emplace_back(x, y, val, id);
    }
    for (int i = 0; i < robot_num; i++)
    {
        int sts;
        scanf("%d%d%d%d", &robot[i].goods, &robot[i].x, &robot[i].y, &robot[i].status);
    }
    for (int i = 0; i < 5; i++)
        scanf("%d%d\n", &boat[i].status, &boat[i].pos);
    char okk[100];
    scanf("%s", okk);
    for (auto g = Goods.begin(); g != Goods.end();)
    {
        if (id - g->birth > max_ttl)
        {
            g = Goods.erase(g);
            g++;
        }
        else
        {
            g++;
        }
    }
    fstream goodlog("./Demo/good.txt", std::ios_base::in | std::ios_base::out | std::ios_base::app);
    goodlog << "goods:" << endl;
    for (auto g : Goods)
    {
        goodlog << "(" << g.x << "," << g.y << ")"
                << " val: " << g.val << " birth: " << g.birth << endl;
    }
    goodlog << to_string(current_frame) << endl;
    goodlog.close();
    return id;
}

void check(int rob_id)
{
    std::ifstream f("./Demo/log" + to_string(rob_id) + ".txt");
    if (f.good())
    {
        f.close();
        cerr << "file exist" << endl;
        return;
    }
    else
        cerr << "nofile" << endl;
    f.close();
    return;
}
void check(string name)
{
    std::ifstream f("./Demo/log" + name + ".txt");
    if (f.good())
    {
        f.close();
        cerr << "file exist" << endl;
        return;
    }
    else
        cerr << "nofile" << endl;
    f.close();
    return;
}

int enCode(pair<int, int> cur, pair<int, int> next)
{
    if (next.first > cur.first)
    {
        return x_plus;
    }
    else if (next.first < cur.first)
    {
        return x_minus;
    }
    else if (next.second > cur.second)
    {
        return y_plus;
    }
    else if (next.second < cur.second)
    {
        return y_minus;
    }
}

int main()
{
    log_p = make_unique<fstream>("./Demo/log.txt", std::ios_base::in | std::ios_base::out);
    if (log_p->good())
    {
        *log_p << "log_p is good" << endl;
    }
    Init();
    for (int j = 0; j < robot_num; j++)
    {
        robot[j].id = j;
    }
    for (int zhen = 1; zhen <= 15000; zhen++)
    {
        last_frame = current_frame;
        current_frame = Input();
        delta_frame = current_frame - last_frame;
        if (delta_frame > 0)
        {
            *log_p << "delta_frame: " << delta_frame << endl;
        }
        /*for (int i = 0; i < robot_num; i++)
        {
            robot[i].poll(i);
        }
        */
       robot[0].poll(0);
        puts("OK");
        fflush(stdout);
    }
    cerr << "end" << endl;
    log_p->close();
    return 0;
}
