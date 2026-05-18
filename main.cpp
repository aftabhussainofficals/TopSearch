#include <windows.h>
#include "SearchEngine.h"
#include "HttpClient.h"
#include "GitHubAPI.h"
#include <iostream>
#include <string>
#include <vector>
#include <thread>
#include <mutex>
#include <algorithm>
#include <chrono>
using namespace std;
using json=nlohmann::json;
string HttpClient::githubToken=[](){
    const char* t=getenv("GITHUB_TOKEN");
    return t?string(t):string();
}();
#define RESET "\033[0m"
#define BOLD "\033[1m"
#define DIM "\033[2m"
#define CYAN "\033[96m"
#define BLUE "\033[94m"
#define GREEN "\033[92m"
#define YELLOW "\033[93m"
#define RED "\033[91m"
#define MAGENTA "\033[95m"
#define WHITE "\033[97m"
static void enableANSI(){
    HANDLE h=GetStdHandle(STD_OUTPUT_HANDLE);
    DWORD mode=0;
    GetConsoleMode(h,&mode);
    SetConsoleMode(h,mode|ENABLE_VIRTUAL_TERMINAL_PROCESSING);
    SetConsoleOutputCP(CP_UTF8);
}
static void cls(){system("cls");}
static void sep(){cout<<DIM<<"  ---------------------------------------------\n"<<RESET;}
static void printBanner(){
    cout<<CYAN<<BOLD
        <<"\n"
        <<"  _____ ___  ____  ____                      _\n"
        <<" |_   _/ _ \\|  _ \\/ ___|  ___  __ _ _ __ ___| |__\n"
        <<"   | || | | | |_) \\___ \\ / _ \\/ _` | '__/ __| '_ \\\n"
        <<"   | || |_| |  __/ ___) |  __/ (_| | | | (__| | | |\n"
        <<"   |_| \\___/|_|   |____/ \\___|\\__,_|_|  \\___|_| |_|\n"
        <<RESET<<"\n";
    cout<<DIM<<"  Search GitHub | GitLab | npm | Stack Overflow\n"<<RESET;
    sep();
    cout<<"\n";
}
static int pickMenu(const string& prompt,const vector<string>& opts,bool showBack=false){
    auto printIt=[&](){
        printBanner();
        cout<<BOLD<<WHITE<<"  "<<prompt<<"\n"<<RESET;
        for(int i=0;i<(int)opts.size();i++)
            cout<<"  "<<CYAN<<"["<<(i+1)<<"]"<<RESET<<" "<<opts[i]<<"\n";
        if(showBack)
            cout<<"  "<<DIM<<"[0] Back"<<RESET<<"\n";
        cout<<"\n  "<<BOLD<<"> "<<RESET;
    };
    cls();printIt();
    int c;
    int lo=showBack?0:1;
    while(!(cin>>c)||c<lo||c>(int)opts.size()){
        cin.clear();cin.ignore(1000,'\n');
        cout<<RED<<"  Invalid, try again..."<<RESET;
        this_thread::sleep_for(chrono::milliseconds(900));
        cls();printIt();
    }
    cin.ignore(1000,'\n');
    return c==0?-1:c-1;
}
static string sv(const json& o,const string& k){
    if(!o.contains(k)||o[k].is_null()) return "N/A";
    return o[k].is_string()?o[k].get<string>():o[k].dump();
}
static int iv(const json& o,const string& k){
    return(o.contains(k)&&!o[k].is_null())?o[k].get<int>():0;
}
static void printReposGitHub(const vector<json>& repos){
    for(int i=0;i<(int)repos.size();i++){
        const json& r=repos[i];
        string owner=r.contains("owner")?sv(r["owner"],"login"):"?";
        string name=sv(r,"name");
        string desc=sv(r,"description");
        string lang=sv(r,"language");
        cout<<"\n  "<<BOLD<<BLUE<<"["<<(i+1)<<"] "<<owner<<" / "<<name<<RESET<<"\n";
        cout<<"  "<<DIM<<"https://github.com/"<<owner<<"/"<<name<<RESET<<"\n";
        cout<<"  "<<YELLOW<<"* "<<iv(r,"stargazers_count")<<" stars  F "<<iv(r,"forks_count")<<" forks";
        if(lang!="N/A") cout<<"  ["<<lang<<"]";
        cout<<RESET<<"\n";
        if(desc!="N/A"&&!desc.empty()) cout<<"  "<<desc<<"\n";
        sep();
    }
}
static void printProfiles(const vector<Profile>& profiles,bool ranked){
    for(int i=0;i<(int)profiles.size();i++){
        const Profile& p=profiles[i];
        cout<<"\n  "<<BOLD;
        if(ranked) cout<<YELLOW<<"#"<<(i+1)<<" ";
        cout<<BLUE<<p.username<<RESET;
        if(!p.name.empty()&&p.name!="N/A")
            cout<<"  "<<DIM<<"("<<p.name<<")"<<RESET;
        cout<<"\n";
        cout<<"  "<<DIM<<"https://github.com/"<<p.username<<RESET<<"\n";
        if(ranked)
            cout<<"  "<<GREEN<<"Score:"<<p.score<<"  Followers:"<<p.followers<<"  Repos:"<<p.repos<<"  Following:"<<p.following<<RESET<<"\n";
        if(!p.bio.empty()&&p.bio!="N/A")
            cout<<"  "<<p.bio<<"\n";
        sep();
    }
}
static void printReposGitLab(const vector<json>& repos){
    for(int i=0;i<(int)repos.size();i++){
        const json& r=repos[i];
        cout<<"\n  "<<BOLD<<MAGENTA<<"["<<(i+1)<<"] "<<sv(r,"name_with_namespace")<<RESET<<"\n";
        cout<<"  "<<DIM<<sv(r,"web_url")<<RESET<<"\n";
        cout<<"  "<<YELLOW<<"* "<<iv(r,"star_count")<<" stars  F "<<iv(r,"forks_count")<<" forks"<<RESET<<"\n";
        string desc=sv(r,"description");
        if(desc!="N/A"&&!desc.empty()) cout<<"  "<<desc<<"\n";
        sep();
    }
}
static void printNpm(const vector<json>& packages){
    for(int i=0;i<(int)packages.size();i++){
        const json& pkg=packages[i]["package"];
        string name=sv(pkg,"name");
        string ver=sv(pkg,"version");
        string desc=sv(pkg,"description");
        string author=pkg.contains("publisher")?sv(pkg["publisher"],"username"):"N/A";
        cout<<"\n  "<<BOLD<<RED<<"["<<(i+1)<<"] "<<name<<RESET<<"  "<<DIM<<"v"<<ver<<RESET<<"\n";
        cout<<"  "<<DIM<<"https://www.npmjs.com/package/"<<name<<RESET<<"\n";
        if(author!="N/A") cout<<"  "<<DIM<<"by "<<author<<RESET<<"\n";
        if(desc!="N/A"&&!desc.empty()) cout<<"  "<<desc<<"\n";
        sep();
    }
}
static void printSO(const vector<json>& questions){
    for(int i=0;i<(int)questions.size();i++){
        const json& q=questions[i];
        bool solved=q.value("is_answered",false);
        cout<<"\n  "<<BOLD<<BLUE<<"["<<(i+1)<<"] "<<sv(q,"title")<<RESET<<"\n";
        cout<<"  "<<DIM<<sv(q,"link")<<RESET<<"\n";
        cout<<"  "<<(solved?GREEN:DIM)<<(solved?"[Answered]":"[Open]")<<RESET
            <<"  Score:"<<iv(q,"score")<<"  Answers:"<<iv(q,"answer_count")<<"  Views:"<<iv(q,"view_count")<<"\n";
        if(q.contains("tags")&&q["tags"].is_array()){
            cout<<"  ";
            for(auto& t:q["tags"]) cout<<CYAN<<"["<<t.get<string>()<<"] "<<RESET;
            cout<<"\n";
        }
        sep();
    }
}
static vector<Profile> rankProfiles(const vector<Profile>& base){
    vector<string> usernames;
    for(auto& p:base) usernames.push_back(p.username);
    vector<Profile> ranked;
    mutex mtx;
    vector<thread> threads;
    for(auto& uname:usernames){
        threads.push_back(thread([&,uname](){
            string data=HttpClient::get("https://api.github.com/users/"+uname);
            if(data.empty()) return;
            try{
                json p=json::parse(data);
                Profile prof;
                prof.username=uname;
                prof.name=p.value("name","");
                prof.bio=p.value("bio","");
                prof.followers=p.value("followers",0);
                prof.repos=p.value("public_repos",0);
                prof.following=p.value("following",0);
                prof.score=prof.followers*3+prof.repos*2+prof.following;
                lock_guard<mutex> lock(mtx);
                ranked.push_back(prof);
            }catch(...){}
        }));
    }
    for(auto& t:threads) t.join();
    sort(ranked.begin(),ranked.end(),[](const Profile& a,const Profile& b){return a.score>b.score;});
    return ranked;
}
int main(){
    enableANSI();
    SearchEngine engine;
    while(true){
        cls();
        printBanner();
        int platform=pickMenu("Select platform:",{"GitHub","GitLab","npm","Stack Overflow","Exit"});
        if(platform==4){cout<<"\n  "<<CYAN<<"Goodbye!\n\n"<<RESET;break;}
        int searchType=0;
        if(platform==0){
            searchType=pickMenu("GitHub search type:",{"Profiles","Repositories","Skills"},true);
            if(searchType==-1) continue;
        }
        cls();printBanner();
        if(platform==0)
            cout<<"  "<<DIM<<"GitHub > "<<vector<string>{"Profiles","Repositories","Skills"}[searchType]<<RESET<<"\n\n";
        cout<<"  "<<BOLD<<"Query (0 to go back): "<<RESET;
        string query;
        getline(cin,query);
        if(query.empty()||query=="0") continue;
        cout<<"\n  "<<DIM<<"Searching...\n"<<RESET;
        engine.search(query,platform,searchType);
        cls();
        printBanner();
        cout<<"  "<<GREEN<<BOLD<<engine.statusMessage<<RESET<<"\n";
        sep();
        if(platform==0){
            if(searchType==1){
                printReposGitHub(engine.repos);
                if(!engine.repos.empty()){
                    cout<<"\n  "<<BOLD<<"Download ZIP? Enter number (0 to skip): "<<RESET;
                    int idx;cin>>idx;cin.ignore(1000,'\n');
                    if(idx>=1&&idx<=(int)engine.repos.size()){
                        auto& r=engine.repos[idx-1];
                        string owner=r.contains("owner")?r["owner"].value("login","?"):"?";
                        string name=r.value("name","?");
                        string branch=r.value("default_branch","main");
                        string zipURL="https://github.com/"+owner+"/"+name+"/archive/refs/heads/"+branch+".zip";
                        string fname=name+".zip";
                        cout<<"  "<<DIM<<"Downloading "<<fname<<"...\n"<<RESET;
                        bool ok=HttpClient::downloadFile(zipURL,fname);
                        cout<<"  "<<(ok?GREEN:RED)<<(ok?"Saved: ":"Failed: ")<<fname<<RESET<<"\n";
                    }
                }
            }else if(searchType==0){
                if(!engine.profiles.empty()){
                    cout<<"\n  "<<BOLD<<"Rank by score? [y/n]: "<<RESET;
                    char c;cin>>c;cin.ignore(1000,'\n');
                    if(c=='y'||c=='Y'){
                        cout<<"  "<<DIM<<"Fetching profiles...\n"<<RESET;
                        engine.profiles=rankProfiles(engine.profiles);
                        cout<<"  "<<GREEN<<"Ranked "<<engine.profiles.size()<<" users.\n"<<RESET;
                        sep();
                        printProfiles(engine.profiles,true);
                    }else{
                        printProfiles(engine.profiles,false);
                    }
                }
            }else{
                printProfiles(engine.profiles,false);
            }
        }else if(platform==1){
            printReposGitLab(engine.repos);
        }else if(platform==2){
            printNpm(engine.packages);
        }else if(platform==3){
            printSO(engine.questions);
        }
        cout<<"\n  "<<DIM<<"Press Enter to continue..."<<RESET;
        cin.ignore(1000,'\n');
    }
    return 0;
}
