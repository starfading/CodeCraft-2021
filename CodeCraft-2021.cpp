#include <iostream>
#include <vector>
#include <string>
#include <unordered_map>
#include <algorithm>
using namespace std;

// hw能买的服务器信息
struct Server{
    string type;              // 型号
    int16_t cpu;              // 核心数
    int16_t memory;           // 内存大小
    int32_t cost_purchase;    // 购买成本
    int16_t cost_day;         // 每天的能耗成本
    double weight;            // 服务器的权重
};

// hw售卖的虚拟机信息
struct VM{
    string type;              // 型号
    int16_t cpu;              // 核心数
    int16_t memory;           // 内存大小
    bool is_double_node;      // 是否是NUMA双节点部署
};

// 每一条用户请求
struct UserRequest{
    string operation;         // 用户操作：“add”或者“del”
    string type;              // 如果是“add”操作，请求部署的虚拟机型号
    uint32_t id_vm;           // 无论是“add”或者“del”，对应的虚拟机的id
};

// 每一天的请求信息
struct DayRequest{
    int32_t num_request;                   // 这一天的用户请求数量
    vector<UserRequest> requests;          // 这一天的用户请求序列
};

vector<Server> SERVERS;                    // hw能买的服务器列表
unordered_map<string, VM> VMS;             // hw售卖的虚拟机列表
vector<DayRequest> DAY_REQUESTS;           // 所有天的请求信息

void Input(){
    string line;
    uint8_t l, r, cnt;

    // 读取服务器信息
    int8_t N;
    getline(cin, line);
    N = stoi(line);
    SERVERS.resize(N);
    Server svr;
    for(int8_t i = 0; i < N; ++i){
        l = 1, r = 1, cnt = 0;
        getline(cin, line);
        while(r < line.length()){
            if(line[r] == ',' || line[r] == ')'){
                string s = line.substr(l, r - l);
                switch (cnt++) {
                    case 0:
                        svr.type = s;
                        break;
                    case 1:
                        svr.cpu = stoi(s);
                        break;
                    case 2:
                        svr.memory = stoi(s);
                        break;
                    case 3:
                        svr.cost_purchase = stoi(s);
                        break;
                    case 4:
                        svr.cost_day = stoi(s);
                        break;
                    default:
                        cerr << "read server info, wrong." << "\n";
                }
                r += 2;
                l = r;
                continue;
            }
            r++;
        }
        // 服务器建档
        SERVERS[i] = svr;
    }

    // 读取虚拟机信息
    int16_t M;
    getline(cin, line);
    M = stoi(line);
    VM vm;
    for(int16_t i = 0; i < M; ++i){
        l = 1, r = 1, cnt = 0;
        getline(cin, line);
        while(r < line.length()){
            if(line[r] == ',' || line[r] == ')'){
                string s = line.substr(l, r - l);
                switch (cnt++) {
                    case 0:
                        vm.type = s;
                        break;
                    case 1:
                        vm.cpu = stoi(s);
                        break;
                    case 2:
                        vm.memory = stoi(s);
                        break;
                    case 3:
                        vm.is_double_node = stoi(s);
                        break;
                    default:
                        cerr << "read vm info, wrong." << "\n";
                }
                r += 2;
                l = r;
                continue;
            }
            r++;
        }
        // 虚拟机建档
        VMS[vm.type] = vm;
    }

    // 读取所有的用户请求
    int16_t T;
    getline(cin, line);
    T = stoi(line);
    DAY_REQUESTS.resize(T);
    UserRequest rqst;
    for(int16_t i = 0; i < T; ++i){
        int32_t R;
        getline(cin, line);
        R = stoi(line);
        DAY_REQUESTS[i].num_request = R;
        DAY_REQUESTS[i].requests.resize(R);
        for(int32_t j = 0; j < R; ++j){
            getline(cin, line);
            if(line[1] == 'a'){
                rqst.operation = "add";
                l = 6, r = 6;
                while(r < line.length()){
                    if(line[r] == ','){
                        string s = line.substr(l, r - l);
                        rqst.type = s;
                        r += 2;
                        l = r;
                        continue;
                    }
                    if(line[r] == ')'){
                        string s = line.substr(l, r - l);
                        rqst.id_vm = stoi(s);
                        break;
                    }
                    r++;
                }
            } else if(line[1] == 'd'){
                rqst.operation = "del";
                l = 6, r = 6;
                while(r < line.length()){
                    if(line[r] == ')'){
                        string s = line.substr(l, r - l);
                        rqst.id_vm = stoi(s);
                        break;
                    }
                    r++;
                }
            } else{
                cerr << "read user request info, wrong." << "\n";
            }
            // 将这一天的这一条用户请求建档
            DAY_REQUESTS[i].requests[j] = rqst;
        }
    }
}

// 已购买的服务器信息
struct MyServer{
    string type;            // 类型
    int16_t cpu_a;          // 节点A剩余的核心数
    int16_t memory_a;       // 节点A剩余的内存大小
    int16_t cpu_b;          // 节点B剩余的核心数
    int16_t memory_b;       // 节点B剩余的内存大小
};

// 部署虚拟机时的操作信息
struct AddVmInfo{
    string type;            // 虚拟机的类型
    int32_t id_server;      // 部署在哪个服务器上
    bool is_double_node;    // 是否双节点部署
    char which_node;        // 如果是单节点部署，那么部署在A点还是B点
};

vector<MyServer> MY_SERVER;                           // 目前购买的所有服务器序列
unordered_map<uint32_t, AddVmInfo> ADD_VM_INFO;       // 部署每一个虚拟机时的操作信息，key是虚拟机id
vector<string> RESULT;                                // 最终的结果序列

void Process(){
    int num_day = DAY_REQUESTS.size();
    // 遍历每一天
    for(int32_t i = 0; i < num_day; ++i){
        // 这天的部署结果
        vector<string> dispose;
        // 这天新购买的服务器（类型，数量）
        unordered_map<string, int32_t> new_purchasing_servers;
        // 这天的请求数量
        int32_t num_request = DAY_REQUESTS[i].num_request;
        // 遍历每个请求
        for(int j = 0; j < num_request; ++j){
            // 当前用户请求
            UserRequest ur = DAY_REQUESTS[i].requests[j];
            if(ur.operation == "add"){
                // 请求的虚拟机信息
                VM vm = VMS[ur.type];
                // 部署用户请求的此虚拟机时产生的操作信息
                AddVmInfo add;
                add.type = ur.type;
                // 在当前服务器是否部署成功
                bool is_disposed = false;
                // 遍历当前所有的服务器
                for(int32_t k = 0; k < MY_SERVER.size(); ++k){
                    if(vm.is_double_node){
                        // 用户请求的虚拟机是双节点部署
                        // 如果当前服务器的容量符合要求
                        if(MY_SERVER[k].cpu_a >= vm.cpu / 2 && MY_SERVER[k].cpu_b >= vm.cpu / 2 &&
                           MY_SERVER[k].memory_a >= vm.memory / 2 && MY_SERVER[k].memory_b >= vm.memory / 2){
                            // 更新这天的部署结果
                            dispose.push_back("(" + to_string(k) + ")");
                            // 更新当前服务器容量
                            MY_SERVER[k].cpu_a -= vm.cpu / 2;
                            MY_SERVER[k].cpu_b -= vm.cpu / 2;
                            MY_SERVER[k].memory_a -= vm.memory / 2;
                            MY_SERVER[k].memory_b -= vm.memory / 2;
                            // 更新部署信息
                            add.is_double_node = true;
                            add.id_server = k;
                            // 部署成功
                            is_disposed = true;
                            break;
                        }
                    } else{
                        // 用户请求的虚拟机是单节点部署
                        if(MY_SERVER[k].cpu_a >= vm.cpu && MY_SERVER[k].memory_a >= vm.memory){
                            // 部署在节点A上
                            // 更新这天的部署结果
                            dispose.push_back("(" + to_string(k)+", "+ "A)");
                            // 更新当前服务器容量
                            MY_SERVER[k].cpu_a -= vm.cpu;
                            MY_SERVER[k].memory_a -= vm.memory;
                            // 更新部署信息
                            add.is_double_node = false;
                            add.id_server = k;
                            add.which_node = 'A';
                            // 部署成功
                            is_disposed = true;
                            break;
                        }
                        if(MY_SERVER[k].cpu_b >= vm.cpu && MY_SERVER[k].memory_b >= vm.memory){
                            // 部署在节点B上
                            // 更新这天的部署结果
                            dispose.push_back("(" + to_string(k)+", "+ "B)");
                            // 更新当前服务器容量
                            MY_SERVER[k].cpu_b -= vm.cpu;
                            MY_SERVER[k].memory_b -= vm.memory;
                            // 更新部署信息
                            add.is_double_node = false;
                            add.id_server = k;
                            add.which_node = 'B';
                            // 部署成功
                            is_disposed = true;
                            break;
                        }
                    }
                }
                // 当前服务器无法部署用户请求的虚拟机
                if(!is_disposed){
                    // 购买一个服务器: svr
                    Server svr;
                    svr = SERVERS[13];
                    new_purchasing_servers[SERVERS[13].type]++;
//                    if(vm.is_double_node){
//                        // 用户请求的虚拟机是双节点部署
//                        for(const auto &server: SERVERS){
//                            if(server.cpu >= vm.cpu && server.memory >= vm.memory){
//                                svr = server;
//                                new_purchasing_servers[server.type]++;
//                                break;
//                            }
//                        }
//                    } else{
//                        // 用户请求的虚拟机是单节点部署
//                        for(const auto &server: SERVERS){
//                            if(server.cpu / 2 >= vm.cpu && server.memory / 2 >= vm.memory){
//                                svr = server;
//                                new_purchasing_servers[server.type]++;
//                                break;
//                            }
//                        }
//                    }
                    MyServer my_server;
                    my_server.cpu_a = svr.cpu / 2;
                    my_server.memory_a = svr.memory / 2;
                    my_server.cpu_b = svr.cpu / 2;
                    my_server.memory_b = svr.memory / 2;
                    my_server.type = svr.type;
                    // 部署虚拟机
                    if(vm.is_double_node){
                        // 用户请求的虚拟机是双节点部署
                        // 更新这天的部署结果
                        dispose.push_back("(" + to_string(MY_SERVER.size()) + ")");
                        // 更新当前服务器容量
                        my_server.cpu_a -= vm.cpu / 2;
                        my_server.cpu_b -= vm.cpu / 2;
                        my_server.memory_a -= vm.memory / 2;
                        my_server.memory_b -= vm.memory / 2;
                        // 更新部署信息
                        add.is_double_node = true;
                        add.id_server = MY_SERVER.size();
                    } else{
                        // 用户请求的虚拟机是单节点部署，部署在新购买服务器的节点A上
                        // 更新这天的部署结果
                        dispose.push_back("(" + to_string(MY_SERVER.size())+", "+ "A)");
                        // 更新当前服务器容量
                        my_server.cpu_a -= vm.cpu;
                        my_server.memory_a -= vm.memory;
                        // 更新部署信息
                        add.is_double_node = false;
                        add.id_server = MY_SERVER.size();
                        add.which_node = 'A';
                    }
                    // 保存新买的服务器信息
                    MY_SERVER.push_back(my_server);
                }
                // 将这一天的这一条虚拟机部署操作信息存档
                ADD_VM_INFO[ur.id_vm] = add;
            } else if(ur.operation == "del"){
                // 用户请求释放虚拟机
                // 定位到这个虚拟机的部署操作信息
                AddVmInfo add = ADD_VM_INFO[ur.id_vm];
                // 回收这个虚拟机占据的服务器资源
                if(add.is_double_node){
                    // 如果虚拟机原来是双节点部署
                    MY_SERVER[add.id_server].cpu_a += VMS[add.type].cpu / 2;
                    MY_SERVER[add.id_server].cpu_b += VMS[add.type].cpu / 2;
                    MY_SERVER[add.id_server].memory_a += VMS[add.type].memory / 2;
                    MY_SERVER[add.id_server].memory_b += VMS[add.type].memory / 2;
                } else{
                    // 如果虚拟机原来是单节点部署
                    if(add.which_node == 'A'){
                        MY_SERVER[add.id_server].cpu_a += VMS[add.type].cpu;
                        MY_SERVER[add.id_server].memory_a += VMS[add.type].memory;
                    } else if(add.which_node == 'B'){
                        MY_SERVER[add.id_server].cpu_b += VMS[add.type].cpu;
                        MY_SERVER[add.id_server].memory_b += VMS[add.type].memory;
                    } else{
                        cerr << "release vm, wrong." << '\n';
                    }
                }
                // 从部署操作序列中删除这个虚拟机的信息
                ADD_VM_INFO.erase(ur.id_vm);
            } else{
                cerr << "user request data, wrong." << "\n";
            }
        }
        // 这一天的请求处理完成，汇总结果到RESULT中
        if(!new_purchasing_servers.empty()){
            // 如果这一天新买了服务器
            // 买了几种类型
            RESULT.push_back("(purchase, " + to_string(new_purchasing_servers.size()) + ")");
            // 每个类型分别是几台
            for(const auto &info: new_purchasing_servers){
                RESULT.push_back("(" + info.first + ", " + to_string(info.second) + ")");
            }
        } else{
            // 如果没有买服务器
            RESULT.push_back("(purchase, " + to_string(0) + ")");
        }
        // 这一天迁移的虚拟机数量
        RESULT.emplace_back("(migration, 0)");
        // 具体迁移信息：每行表示一个虚拟机的迁移，格式为(虚拟机 ID, 目的服务器 ID)或(虚拟机 ID, 目的服务器 ID, 目的服务器节点)
        // 暂时不迁移

        // 这一天的部署结果
        for(const auto &info: dispose){
            RESULT.push_back(info);
        }
    }
}

void Output(){
    for(const auto &info: RESULT){
        cout << info << "\n";
    }
}

int main(){
    Input();
    Process();
    Output();
    return 0;
}