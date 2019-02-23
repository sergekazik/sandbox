#include <assert.h>
#include <numeric>
#include <array>
#include <map>
#include <typeinfo>

#include <iostream>     // std::cout
#include <algorithm>    // std::copy_if, std::distance
#include <vector>       // std::vector
#include <string>

#include <sstream>
#include <iterator>
#include <numeric>

//#include <random>
#include <future>
#include <memory>
#include <functional> // bind
#include <tuple>

#include <stdio.h>
#include <string.h>
#include <unistd.h>

#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <unistd.h>
#include <dirent.h>
#include <sys/types.h>

using namespace std;
typedef char *tokarr[8];

#if 1
int main()
{
    uint16_t x = 65521;
    printf("x = %d, int16_t x = %d\n", x, (int16_t) x);

}

#elif 1 // Determine programmatically if a program is running
pid_t proc_find(const char* name)
{
    DIR* dir;
    struct dirent* ent;
    char* endptr;
    char buf[512];

    if (!(dir = opendir("/proc"))) {
        perror("can't open /proc");
        return -1;
    }

    while((ent = readdir(dir)) != NULL) {
        /* if endptr is not a null character, the directory is not
         * entirely numeric, so ignore it */
        long lpid = strtol(ent->d_name, &endptr, 10);
        if (*endptr != '\0') {
            continue;
        }

        /* try to open the cmdline file */
        snprintf(buf, sizeof(buf), "/proc/%ld/cmdline", lpid);
        // printf("checking %s\n", buf);
        FILE* fp = fopen(buf, "r");

        if (fp) {
            if (fgets(buf, sizeof(buf), fp) != NULL) {
                /* check the first token in the file, the program name */
                char* first = strtok(buf, " ");
                // printf("first token = %s\n", first);
                // if (!strcmp(first, name))
                if (strstr(first, name))
                {
                    fclose(fp);
                    closedir(dir);
                    return (pid_t)lpid;
                }
            }
            fclose(fp);
        }
    }
    closedir(dir);
    return -1;
}


int main(int argc, char* argv[])
{
    if (argc == 1) {
        printf("usage: %s name1 name2 ...\n", argv[0]);
        return 1;
    }

    for(int i = 1; i < argc; ++i) {
        pid_t pid = proc_find(argv[i]);
        if (pid == -1) {
            printf("%s: not found\n", argv[i]);
        } else {
            printf("%s: %d\n", argv[i], pid);
        }
    }

    return 0;
}

#elif 1
int main()
{
    char line[] = "THIS is line";
    const char delim[] = " ";
    char *value = strtok(line, delim);
    if (value != NULL)
    {
        printf("TESTBLE:  update %s with \"%s\"\n", value, strtok(NULL, delim));
    }

    // cut CR nad LF
    for (; strlen(line) && strchr("\n\r", line[strlen(line)-1]); line[strlen(line)-1] = '\0');

    return 0;
}

#elif 1
static volatile char server_notify_listener_done = 0;
static void* server_notify_listener(void *data __attribute__ ((unused)))
{
    while (!server_notify_listener_done)
    {
        printf("server_notify_listener listening....\n");
        sleep(1);
    }

// done:
    printf("server_notify_listener_done\n");
    fflush(stdout);
    pthread_exit(NULL);
    return (void*) 0;
}

int main()
{
    pthread_t thread_id;
    if (0 != pthread_create(&thread_id, NULL, server_notify_listener, NULL))
    {
        printf("failed to create pthread %s (%d)", strerror(errno), errno);
        return -5;
    }
    printf("\nCMD_WAIT_NOTIFICATIONS listening from server...\npress ENTER to terminate and continue\\>");
    getchar();
    server_notify_listener_done = 1;
}

int split(char* str, tokarr &tarr)
{

  char * pch;
  printf ("Splitting string \"%s\" into tokens:\n",str);
  pch = strtok (str,","); // strtok (str," ,.-");
  int count = 0;
  while (pch != NULL)
  {
    printf ("%s\n",pch);
    tarr[count]=pch;
    count++;
    pch = strtok(NULL, ",");

  }
  return count;

//  example
//        char str[] ="udp,1001,17";
//        tokarr tarr;
//        int num = split(str, tarr);
//        for (int i = 0; i < num; i++)
//            printf ("%s\n",tarr[i]);
//        return 0;
}

void test_convert(void);

#elif 1
unsigned char * scramble_unscramble(unsigned char* src, int len)
{
    for (int i = 0; i < len; i++)
    {
        src[i] ^= ((i*2)%0xff);
        if ('a' <= src[i] && src[i] <= 'z')
            printf("%c ", src[i]);
        else
            printf("%02x ", src[i]);
    }
    printf("\n");
    return src;
}

int main ()
{
    unsigned char str1[] = "\x01\x00\x01\x00\x01\x00";
    unsigned char str2[] = "hello vasya";

    scramble_unscramble(str1, 6);
    scramble_unscramble(str1, 6);
    scramble_unscramble(str1, 6);
    scramble_unscramble(str1, 6);
    scramble_unscramble(str2,11);
    scramble_unscramble(str2,11);
    scramble_unscramble(str2,11);
    scramble_unscramble(str2,11);
}

#elif 1
int main ()
{
  char str[] ="udp,1001,17";
  char * pch;
  printf ("Splitting string \"%s\" into tokens:\n",str);
  pch = strtok (str,","); // strtok (str," ,.-");
  while (pch != NULL)
  {
    printf ("%s\n",pch);
    pch = strtok (NULL, " ,.-");
  }
  return 0;
}

#elif 1
vector<string> split(const char *phrase, string delimiter){
    vector<string> list;
    string s = string(phrase);
    size_t pos = 0;
    string token;
    while ((pos = s.find(delimiter)) != string::npos) {
        token = s.substr(0, pos);
        list.push_back(token);
        s.erase(0, pos + delimiter.length());
    }
    list.push_back(s);
    return list;
}

int main()
{
#if TEST_CONVERT
    test_convert();
#elif TEST_POTOCK
    std::string s = "udp,1001";
    std::string delimiter = ",";

    size_t pos = 0;
    std::string token;
    while ((pos = s.find(delimiter)) != std::string::npos) {
        token = s.substr(0, pos);
        std::cout << token << std::endl;
        s.erase(0, pos + delimiter.length());
    }
    std::cout << s << std::endl;
#else
    vector<string> list = split("udp,1001", ",");

    cout << list.size() << endl;
    for (unsigned int i = 0; i < list.size(); i++)
        std::cout << list[i] << endl;

#endif

}
#else
using namespace std;
using namespace std::placeholders;

template <typename T1, typename T2>
auto add(T1 first, T2 second) -> decltype(first + second)
{
    return first + second;
}

struct MyArray {
    MyArray(): myData{1,2,3,4,5} {}
    int myData[5];
} array;

void asd() {
    enum {N = 3};
    std::array<int,N> a{10,20,30}; // uniform initialization syntax

    decltype(a) b;
    auto endb = copy_if(begin(a), end(a), begin(b), std::bind(less<int>(),std::placeholders::_1,15));

    cout << "\n// uniform initialization syntax - copy ";
    copy(begin(a), end(a), std::ostream_iterator<int>(cout, ", "));
    cout << "with bind less 15" << endl;

    copy(begin(b),endb,ostream_iterator<int>(cout,"\n")); // 10
}

template<typename T> using smap = map<string,T>;


// Trailing  Return  Type  (-­‐>)
// The  expression   is  not evaluated
template<typename T1, typename T2>
auto min(T1 t1, T2 t2)->decltype(t1+t2) { // Promote to wider type
    return t1 < t2 ? t1 : t2;
}

template<typename Tf>
auto freedome(Tf arg)
{
    cout << arg << "[" << typeid(arg).name() << "]" << " is "
         << (*typeid(arg).name() == 'i' ? "integer" : *typeid(arg).name() == 'd' ? "double" : "something like string")
         << endl;
}

function<int(int)> addx(int x) {
    return [x](int y)->int {return x+y;};
}


// Capturing   this
class Foo {
    string s;
public:
    Foo(const string& s) : s(s) {}
    function<string(void)> greet() {
        return [this]{return "hello " + s;};
    }
};

// Mutable  Lambdas
void algo(vector<int>& v) { // Function from Stroustrup 4th
    int count = v.size()+1;
    generate(begin(v), end(v),
             [count]() mutable {return --count;}); // local mods
}

class C {
public:
    C() = default;
    C(const C&) {cout << "copy constructor\n";}
    C(C&&) {cout << "move constructor\n";}
    C& operator=(const C&) {cout << "copy assignment\n"; return *this;}
    C& operator=(C&&) {cout << "move assignment\n"; return *this;}
};

// Variadic Templates
// Allows  variable-­‐length  argument  lists  of  mixed  types
void display() {}  // To stop the recursion

template<typename T, typename... Rest>
void display(T head, Rest... rest) {
    cout << typeid(T).name() << ": " << head << endl;
    display(rest...); // parameter pack
}


// std::bind
class Foo1 {
    int x;
public:
    Foo1(int n) : x(n) {}
    int f() const {return x;}
    int g(int y) const {return x+y;}
    void display() const {cout << x << endl;}
};

using MyTuple = tuple<int,string>;
MyTuple incr(const MyTuple& t) {
    return MyTuple(get<0>(t)+1, get<1>(t) + "+one");
}

void deleter(FILE* f) {
    fclose(f);
    cout << "FILE* closed\n";
}
void client_server_foo(void);

int main()
{

    std::cout <<  add(1,1) << endl;
    std::cout <<  add(1,1.1) << endl;
    std::cout <<  add(1000LL,5) << endl;
    // std::cout <<  add("qwe",3) << endl;

    auto myLambda= []{string st= {"lambda function"}; return st;};
    std::cout <<  myLambda() << endl;

    vector<int> vec={1,2,3,4,5};
    for (auto& v: vec) cout << v << ",", v *= 2; cout << endl;
    for (auto v: vec) cout << v << ","; cout << endl;

    // USING - instead of typedef
    using intseq = int[5];
    intseq a;
    for (int i = 0; i < 5; ++i)
    a[i] = 5-i;
    for (int i = 0; i < 5; ++i)
    cout << a[i] << ' ';
    std::cout <<  endl;

    // using and template - see declaration before main
    { // template<typename T> using smap = map<string,T>;
        smap<float> mymap {{"one",1},{"two",2.5}};
        for (const auto &p: mymap)
            cout << p.first << ',' << p.second << endl;
    }

    { // Trailing  Return  Type  (-­‐>)
        auto x = min(1.1, 2);
        cout << x << ", " << typeid(x).name() << endl;
        auto y = min(4, 2);
        cout << y << ", " << typeid(y).name() << endl;
    }
    freedome(1);
    freedome(123.123);
    freedome("herlo");

    std::istringstream str("0.1 0.2 0.3 0.4");
    std::partial_sum(std::istream_iterator<double>(str),
                      std::istream_iterator<double>(),
                      std::ostream_iterator<double>(std::cout, " "));

    cout << endl;
    {
        cout << endl << "// Lambda  Expressions\n" << "// copy_if example with lambda" << endl;
        enum {N = 3};
        std::array<int,N> a{10,20,30};
        decltype(a) b;
        auto endb = std::copy_if(begin(a), end(a), begin(b),
                            [](int x){return x < 25;});
        copy(begin(b), endb, ostream_iterator<int>(cout, "\n"));
    }

    // Lambda  Expressions
    // [<capture>]  ( <args>)  { <body}  -­‐> <return  type>
    {
        cout << endl << "// [<capture>]  ( <args>)  { <body}  -­‐> <return  type>" << endl;
        vector<int> v{0,1,2,3,4,5};
        transform(begin(v), end(v), begin(v), [](int n){static auto prev = 0; int nove = n + prev; prev = nove; return nove;});
        copy(begin(v), end(v), ostream_iterator<int>(cout, " "));
        cout << endl;
        sort(begin(v), end(v), [](int m, int n){return m > n;});
        copy(begin(v), end(v), ostream_iterator<int>(cout, " "));
        cout << endl;
    }

    // Lambda  With  Capture  (“Closures”)
    //    [=] captures   everything visible   by  value
    //    [&] captures   everything by  reference   (careful!)
    //    Can  capture   individual   variables:   [=x,&y]...
    //    Globals aren’t   (and  don’t   need   to  be)  captured
    {
        auto f = addx(10);
        cout << f(3) << endl; // output: 13
    }

    // Recursive  Lambdas
    // Captures   the  function   name
    function<int(int)> fib =
        [&fib](int n) {return n < 2 ? n : fib(n-1) + fib(n-2);};
    for (auto n: {0,1,2,3,4,5,6,7})
        cout << fib(n) << ", ";


    // Capturing   this
    Foo f("Larry");
    auto g = f.greet();
    cout << endl << g() << endl;


    // Mutable  Lambdas
    vector<int> v(11);
    algo(v);
    copy(begin(v),end(v),ostream_iterator<int>(cout," "));
    cout << endl;


    // Range-­‐based   for and  Higher-­‐dim  Arrays
    int zu[][3] = {{1,2,3},{4,5,6},{7,8,9}};
    for (const auto &row: zu) {
        cout << "sizeof(row): " << sizeof(row)
             << " row val " << accumulate(begin(row), end(row), 0, [](auto x, auto y) {return x+y;}) << endl;
        for (const auto &x: row)
            cout << "sizeof(" << x << "): " << sizeof(x)
                 << " val: " << x << endl;
    }


    // Uniform  Initialization  Syntax
    {
        int a[]{101,201,301};
        cout << "out with copy: ";
        std::copy(begin(a), end(a), std::ostream_iterator<int>(cout, ", "));
        cout << endl;

        cout << "out with range loop: ";
        for (const auto b : a) cout << b << ", ";
        cout << endl;

        vector<int>  v{1,2,3};
        cout << "vector by copy: ";
        std::copy(begin(v), end(v), std::ostream_iterator<int>(cout, ", "));
        cout << endl;

        v  =  {4,5,6};        //  Assigning  from  an  initializer_list
        cout << "reassigned value vector by copy: ";
        std::copy(begin(v), end(v), std::ostream_iterator<int>(cout, ", "));
        cout << endl;

        v.insert(end(v),{7,8,9});     //  Append
        cout << "appended vector by copy: ";
        std::copy(begin(v), end(v), std::ostream_iterator<int>(cout, ", "));
        cout << endl;

        Foo foo{"the one"};
        auto f = foo.greet();
        cout << f() << endl;

        map<string,int>  m{{"one_hundred and", 1},{"two_thousand and", 2}};
        for (const auto &p: m) // for (auto p: m)
            cout << p.first << " " << p.second << endl;

        // init function by call
        // f({1,2,3});
    }

    // static_assert
    #define CHAR_BIT 8
    static_assert(CHAR_BIT == 8,"8 bits per byte required");
    static_assert(sizeof(char*) == 8,"64-bit architecture required");
    const int N = 10;
    static_assert(N > 2,"N must be > 2");

    std::cout << endl <<  "---------------------" << endl;
    std::vector<std::string> svec = { "red",
                                      "green",
                                      "blue" };
    std::vector<int> ivec = { 1, 2, 3, 4};
    auto adder  = [](auto op1, auto op2){ return op1 + op2; };
    std::cout << "int result : "
              << std::accumulate(ivec.begin(),
                                 ivec.end(),
                                 0,
                                 adder )
              << "\n";
    std::cout << "string result : "
              << std::accumulate(svec.begin(),
                                 svec.end(),
                                 std::string(""),
                                 adder )
              << "\n";


    cout << endl << "// Variadic Templates\n"
         << "// Allows  variable-­‐length  argument  lists  of  mixed  types" << endl;
    display("one",1,2.345);

    asd();


    cout << endl << "// std::bind and Member Functions with for_each" << endl;
    Foo1 obj(5);
    auto f1 = bind(&Foo1::f,_1); // “Unbound method” - doesn't contain object
    cout << "Unbound method > " << f1(obj) << endl; // f1 is called with created obj(5) - prints 5

    auto f2 = bind(&Foo1::g,obj,_1); // “Bound method” - bound to obj
    cout << "Bound method > " << f2(3) << endl; // f2 called with the parameter to add

    std::array<Foo1,3> z = {Foo1(1),Foo1(2),Foo1(3)};      // this is array of objects
    for_each(z.begin(),z.end(),bind(&Foo1::display,_1)); // function to call and placeholder of the element

    vector<Foo1*> vct = {new Foo1(4), new Foo1(5)};     // this is array of the pointers to the function - so need to be cleaned
    for_each(vct.begin(),vct.end(),bind(&Foo1::display,_1)); // Just works!
    for_each(vct.begin(),vct.end(),[](Foo1* p){delete p;}); // Clean-up


    //////////////////////////////////////////////////////////////////////////////////////////
    cout << endl << "// Tuples - generalization of std::pair" << endl;
    {
        MyTuple tup0(1,"text");
        auto tup1 = incr(tup0);
        cout << get<0>(tup1) << ' ' << get<1>(tup1) << endl;

        auto tup2 = std::make_tuple(2, string("text+one"));
        assert(tup1 == tup2);

        int n;
        string s;
        std::tie(n,s) = incr(tup2);  // Tuple assignment ??
        cout << n << ' ' << s << endl;
    }

    //////////////////////////////////////////////////////////////////////////////////////////
    {   cout << endl << "// unique_ptr Example  1" << endl;
        class Trace {
            int x;
        public:
            Trace() : x(5) { cout << "ctor\n"; }
            Trace(int a) : x(a) { cout << "ctor\n"; }
            ~Trace() { cout << "dtor\n"; }
            int get() const { return x; }
        };
        unique_ptr<Trace> p(new Trace(24));
        cout << p->get() << '\n';
    }

    //////////////////////////////////////////////////////////////////////////////////////////
    {   cout << endl << "// unique_ptr Example 2 - vertor of pointers" << endl;
        class Foo {
        public:
            Foo(){}
            ~Foo() {
                cout << "destroying a Foo " << mCount << endl;
            }
            virtual void saysomething() {cout << "Foo saying... " << mCount << endl;}

        protected:
            size_t mCount = clock(); //    In-­‐class  initializers
        };

        //////////////////////////////////////////////////////////////////////////////////////////
        { // scope to exit
            vector<unique_ptr<Foo>> v;
            v.push_back(unique_ptr<Foo>(new Foo));
            v.push_back(unique_ptr<Foo>(new Foo));
            v.push_back(unique_ptr<Foo>(new Foo));

            for(auto& p: v) p->saysomething();
        }
        class FooD : Foo {
        public:
            void saysomething() override final {cout << "FooD is saying... " << mCount << " end calling base class" << endl; Foo::saysomething();}
        };

        class Frue : FooD {
        public:
            // void saysomething() - this will be error since it's final in FooD
            void hello() {cout << "hello\n";}
        };

        cout << endl << "// unique_ptr Example 3 - array of 3 ptrs" << endl;
        unique_ptr<FooD[]> p(new FooD[3]);
        p[1].saysomething();

        unique_ptr<Frue> fp(new Frue);
        fp->hello();
    }

    //////////////////////////////////////////////////////////////////////////////////////////
    {   cout << endl << "// Custom deleter" << endl;
        // The following uses a deleter, but no wrapper class!
        FILE* f = fopen("deleter1.sample.txt", "r");
        assert(f);

        unique_ptr<FILE, void(*)(FILE*)> anotherFile(f, &deleter); // this will call custom deleter

        // Could just do this instead (but there would be no trace)
        FILE* f2 = fopen("deleter1.sample.txt", "r");
        assert(f2);

        unique_ptr<FILE, int(*)(FILE*)> the3rdFile(f2, &fclose); // this will call standard fclose on exit out of scope
    }

    //////////////////////////////////////////////////////////////////////////////////////////
    {   cout << endl << "// Automatic  Cleanup  with  shared_ptr" << endl;
        class Resource {
            // Note: no public constructors!
            Resource() = default;
        public:
            static Resource* Create() {
                // Factory method
                Resource* p = new Resource;
                return p;
            }
            ~Resource() { cout << "Resource destroyed\n"; }
        };
        // A Client uses a Resource
        class Client
        {
        public:
            Client(shared_ptr<Resource> p) : pRes(p){}
            ~Client() { cout << "Client object destroyed\n"; }
        private:
            shared_ptr<Resource> pRes;
        };

        // Create a Resource to be shared:
        shared_ptr<Resource> pR(Resource::Create());
        cout << "pR.use_count() " << pR.use_count() << endl;        // count is 1

        // Use the Resource in 2 clients:
        Client b1(pR);
        cout << "pR.use_count() " << pR.use_count() << endl;
        Client b2(pR);
        cout << "pR.use_count() " << pR.use_count() << endl;
        // b2.~Client() will reduce count to 2.
        // b1.~Client() will reduce count to 1.
        // pR.~shared_ptr<Resource>() will reduce the count to 0.
        // ...after which the Resource will self-destruct.
    }

    // async example - works!
    // client_server_foo();

//    New  Class-­‐Related  Features
//    In-­‐class  initializers
//    Inheritance  control  with   final and   override
//    Delegating/Forwarding  constructors
//    Inheriting  constructors
//    Copy  control  with  = default,  =delete
//    Move  semantics  with   rvalue references


    return 0;
}


#if 0
#include <iostream>
#include <stdio.h>
#include <stdlib.h>

#include <signal.h>
#include <stdio.h>
#include <string.h>
#include <sys/time.h>
#include <unistd.h>

#if 1
int main()
{
    const char *str = "abcdef";
    if (str[2] == 2[str])
        cout << strlen(str)+2 << " vs " << strlen(str+2) << " " << __LINE__ << endl;
    return 0;
}

#elif 0

void timer_handler (int signum)
{
    (void) signum;
    static int count = 0;
    printf ("timer expired %d times\n", ++count);
}

int start_timer (int ms)
{
    struct sigaction sa;
    struct itimerval timer;

    /* Install timer_handler as the signal handler for SIGVTALRM. */
    memset (&sa, 0, sizeof (sa));
    sa.sa_handler = &timer_handler;
    sigaction (SIGVTALRM, &sa, NULL);

    /* Configure the timer to expire after ms msec... */
    timer.it_value.tv_sec = 0;
    timer.it_value.tv_usec = ms*1000;
    /* ... and every ms msec after that. */ // - no repeat
    timer.it_interval.tv_sec = 0;
    timer.it_interval.tv_usec = ms*1000;;
    /* Start a virtual timer. It counts down whenever this process is executing. */
    setitimer (ITIMER_VIRTUAL, &timer, NULL);

    return 0;
}

int main()
{
    int count = 10;
    start_timer(500);
    printf("no exit from here\n");
    // getchar();
    while (1)
    {
    }
    return 0;
}

int main2()
{

    cout << "Hello World!" << endl;
    const char * hello = "hello";
    printf("%-10s %s\n", hello, hello);

    for (int i = 5; i < 15; i++)
    {
        printf("%-2d) %-40s*\n", i, "hell");
    }

    return 0;
}

#endif
#endif

template <typename RandomIt>
int parallel_sum(RandomIt beg, RandomIt end)
{
    auto len = end - beg;
    if (len < 1000)
        return std::accumulate(beg, end, 0);

    RandomIt mid = beg + len/2;
    auto handle = std::async(std::launch::async,
                             parallel_sum<RandomIt>, mid, end);
    int sum = parallel_sum(beg, mid);
    return sum + handle.get();
}

#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <errno.h>
#include <string.h>
#include <sys/types.h>
#include <time.h>

static int server_sock(void)
{
    int listenfd = 0, connfd = 0;
    struct sockaddr_in serv_addr;

    char sendBuff[1025];
    time_t ticks;

    listenfd = socket(AF_INET, SOCK_STREAM, 0);
    memset(&serv_addr, '0', sizeof(serv_addr));
    memset(sendBuff, '0', sizeof(sendBuff));

    serv_addr.sin_family = AF_INET;
    serv_addr.sin_addr.s_addr = htonl(INADDR_ANY);
    serv_addr.sin_port = htons(5000);

    bind(listenfd, (struct sockaddr*)&serv_addr, sizeof(serv_addr));

    listen(listenfd, 10);

    // while(1)
    {
        connfd = accept(listenfd, (struct sockaddr*)NULL, NULL);

        ticks = time(NULL);
        snprintf(sendBuff, sizeof(sendBuff), "%.24s\r\n", ctime(&ticks));
        write(connfd, sendBuff, strlen(sendBuff));

        close(connfd);
        sleep(1);
     }

    close(listenfd);
    return 0;
}

int client_sock(void)
{
    int sockfd = 0, n = 0;
    char recvBuff[1024];
    struct sockaddr_in serv_addr;
    const char *argv1 = "127.0.0.1";

    memset(recvBuff, '0',sizeof(recvBuff));
    if((sockfd = socket(AF_INET, SOCK_STREAM, 0)) < 0)
    {
        printf("\n Error : Could not create socket \n");
        return 1;
    }

    memset(&serv_addr, '0', sizeof(serv_addr));

    serv_addr.sin_family = AF_INET;
    serv_addr.sin_port = htons(5000);

    if(inet_pton(AF_INET, argv1, &serv_addr.sin_addr)<=0)
    {
        printf("\n inet_pton error occured\n");
        return 1;
    }

    if( connect(sockfd, (struct sockaddr *)&serv_addr, sizeof(serv_addr)) < 0)
    {
       printf("\n Error : Connect Failed \n");
       return 1;
    }

    while ( (n = read(sockfd, recvBuff, sizeof(recvBuff)-1)) > 0)
    {
        recvBuff[n] = 0;
        cout << "client sock recieved: " << recvBuff << endl;
    }

    if(n < 0)
    {
        printf("\n Read error \n");
    }

    return 0;
}

void client_server_foo(void)
{
    cout << endl << "// async sample" << endl;
    std::vector<int> v(10000, 2);
    std::cout << "The sum is " << parallel_sum(v.begin(), v.end()) << '\n';


    // TCP socket client server sample
    cout << endl << "// async Client-Server sample" << endl;
    cout << "// let's try running server asynchronously\n";
    auto server = std::async(std::launch::async, server_sock);
    cout << "// running client syncrhonously\n";
    int ret = client_sock();
    cout << "client exited with exit code: " << ret << endl;
    cout << "getting from server exit code: " << server.get() << endl;
}
#endif
