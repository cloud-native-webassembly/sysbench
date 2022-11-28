
/**
 * a real udf from https://yuque.alibaba-inc.com/odps/ecgg7k/sdgve5
 */
#include <stdlib.h>
#include <string.h>
#include <stdbool.h>

char* yanhao_urls =
    "{ \
            '买家足迹': 'https://www.taobao.com/markets/footmark/tbfoot' # 买家足迹 \
            , '购买记录': 'https://buyertrade.taobao.com/trade/itemlist/list_bought_items.htm' # 购买记录 \
            , '我的评价': 'https://rate.taobao.com' # 我的评价 \
            , '性别年龄': 'https://member1.taobao.com/member/fresh/account_profile.htm' # 性别年龄 \
            , '体检中心': 'https://passport.taobao.com/ac/h5/appeal_center.htm' # 体检中心 \
            , '淘气值': 'https://pages.tmall.com/wow/88vip/act/taoqizhi' # 淘气值 \
        }";

int entry(int param_index){
    
}

bool evaluate(char* url) {
    if (url == NULL) {
        return false;
    }
    if (strstr(yanhao_urls, url) != NULL) {
        return true;
    } else {
        return false;
    }
}
