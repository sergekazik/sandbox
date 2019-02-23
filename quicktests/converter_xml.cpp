#include <assert.h>
#include <string>
#include <numeric>
#include <array>
#include <map>
#include <typeinfo>

#include <iostream>     // std::cout
#include <algorithm>    // std::copy_if, std::distance
#include <vector>       // std::vector

#include <sstream>
#include <iterator>
#include <numeric>

//#include <random>
#include <future>
#include <memory>
#include <functional> // bind
#include <tuple>

#include "string.h"
using namespace std;
using namespace std::placeholders;

const char *xml1="<ap_list>"
    "<ap>"
        "<index>0</index>"
        "<ssid>RingSetup01</ssid>"
        "<rssi>28</rssi>"
        "<nw_type>infra</nw_type>"
        "<security>none</security>"
        "<channel>1</channel>"
    "</ap>"
    "<ap>"
        "<index>1</index>"
        "<ssid>aaron_ap_1533</ssid>"
        "<rssi>31</rssi>"
        "<nw_type>infra</nw_type>"
        "<security>wpa-personal</security>"
        "<channel>6</channel>"
    "</ap>"
"</ap_list>";


const char* tagsin[] = {"<ap_list>", "<ap>", "<index>", "</index>", "<ssid>", "</ssid>", "<rssi>", "</rssi>","<nw_type>",
                        "</nw_type>", "<security>", "</security>", "<channel>", "</channel>","</ap>", "</ap_list>"};
const char* tagsout[] = {"<1>", "<2>", "<3>", "</3>", "<4>", "</4>", "<5>", "</5>","<6>",
                         "</6>", "<7>", "</7>", "<8>", "</8>","</2>", "</1>"};


char *convert_xml(const char* xml)
{
    if (!xml || !strlen(xml))
        return NULL;
    std::string in = string(xml);

    char *newx = strdup(xml);
    if (!newx)
        return NULL;

    for (int i = 0; i < (int) (sizeof(tagsin)/sizeof(const char*)); i++)
    {
        const char *pStart = NULL;
        while (NULL != (pStart = strstr(newx, tagsin[i])))
        {
            in.replace((pStart - newx), strlen(tagsin[i]), tagsout[i]);
            strcpy(newx, in.c_str());
        }

    }

    cout << in.c_str() << endl;
    cout << "the original " << strlen(xml) << " new len " << strlen(newx) << endl;

    return newx;
}

void test_convert(void)
{
    convert_xml(xml1);
}
