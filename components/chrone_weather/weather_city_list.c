/**
 * @file weather_city_list.c
 * @brief 心知 location 白名单（硬编码）。增删时请用官方城市搜索接口核对 location 字符串。
 */

#include "weather_city_list.h"
#include <strings.h>
#include <string.h>

/*
 * 说明：
 * - id 须与心知 API 返回的 location 查询条件一致，一般为小写全拼。
 * - 重名城市用心知文档推荐形式，如 "shanxi yulin" / "shaanxi yulin"（请自行用 search.json 验证）。
 * - 以下为「直辖市 + 省会 + 自治区首府 + 常用地级市」子集，可按需在同表继续追加至全国地级规模。
 */
static const weather_city_entry_t kWeatherCities[] = {
    /* 直辖市 */
    { "beijing", "北京" },
    { "tianjin", "天津" },
    { "shanghai", "上海" },
    { "chongqing", "重庆" },

    /* 省会、自治区首府（常用拼音写法） */
    { "shijiazhuang", "石家庄" },
    { "taiyuan", "太原" },
    { "huhehaote", "呼和浩特" },
    { "shenyang", "沈阳" },
    { "changchun", "长春" },
    { "haerbin", "哈尔滨" },
    { "nanjing", "南京" },
    { "hangzhou", "杭州" },
    { "hefei", "合肥" },
    { "fuzhou", "福州" },
    { "nanchang", "南昌" },
    { "jinan", "济南" },
    { "zhengzhou", "郑州" },
    { "wuhan", "武汉" },
    { "changsha", "长沙" },
    { "guangzhou", "广州" },
    { "nanning", "南宁" },
    { "haikou", "海口" },
    { "chengdu", "成都" },
    { "guiyang", "贵阳" },
    { "kunming", "昆明" },
    { "lasa", "拉萨" },
    { "xian", "西安" },
    { "lanzhou", "兰州" },
    { "xining", "西宁" },
    { "yinchuan", "银川" },
    { "wulumuqi", "乌鲁木齐" },

    /* 计划单列市 / 常见强二线 */
    { "shenzhen", "深圳" },
    { "dalian", "大连" },
    { "qingdao", "青岛" },
    { "ningbo", "宁波" },
    { "xiamen", "厦门" },

    /* 东部 / 中部 常用地级市（示例批量，可继续扩展） */
    { "suzhou", "苏州" },
    { "wuxi", "无锡" },
    { "changzhou", "常州" },
    { "nantong", "南通" },
    { "yangzhou", "扬州" },
    { "xuzhou", "徐州" },
    { "wenzhou", "温州" },
    { "jiaxing", "嘉兴" },
    { "huzhou", "湖州" },
    { "shaoxing", "绍兴" },
    { "jinhua", "金华" },
    { "taizhou", "台州" },
    { "weihai", "威海" },
    { "yantai", "烟台" },
    { "weifang", "潍坊" },
    { "zibo", "淄博" },
    { "jining", "济宁" },
    { "linyi", "临沂" },
    { "tangshan", "唐山" },
    { "baoding", "保定" },
    { "langfang", "廊坊" },
    { "handan", "邯郸" },
    { "qinhuangdao", "秦皇岛" },
    { "datong", "大同" },
    { "linfen", "临汾" },
    { "huainan", "淮南" },
    { "wuhu", "芜湖" },
    { "maanshan", "马鞍山" },
    { "fuyang", "阜阳" },
    { "ganzhou", "赣州" },
    { "jingdezhen", "景德镇" },
    { "jiujiang", "九江" },
    { "zaozhuang", "枣庄" },
    { "dongying", "东营" },
    { "liaocheng", "聊城" },
    { "dezhou", "德州" },
    { "kaifeng", "开封" },
    { "luoyang", "洛阳" },
    { "anyang", "安阳" },
    { "xinxiang", "新乡" },
    { "nanyang", "南阳" },
    { "xinyang", "信阳" },
    { "xiangyang", "襄阳" },
    { "yichang", "宜昌" },
    { "jingzhou", "荆州" },
    { "huangshi", "黄石" },
    { "zhuzhou", "株洲" },
    { "yueyang", "岳阳" },
    { "hengyang", "衡阳" },
    { "chenzhou", "郴州" },
    { "zhanjiang", "湛江" },
    { "maoming", "茂名" },
    { "jiangmen", "江门" },
    { "zhaoqing", "肇庆" },
    { "shanwei", "汕尾" },
    { "heyuan", "河源" },
    { "qingyuan", "清远" },
    { "shantou", "汕头" },
    { "shaoguan", "韶关" },
    { "meizhou", "梅州" },
    { "foshan", "佛山" },
    { "dongguan", "东莞" },
    { "zhongshan", "中山" },
    { "zhuhai", "珠海" },
    { "huizhou", "惠州" },
    { "liuzhou", "柳州" },
    { "guilin", "桂林" },
    { "beihai", "北海" },
    { "sansha", "三沙" },
    { "danzhou", "儋州" },
    { "mianyang", "绵阳" },
    { "deyang", "德阳" },
    { "nanchong", "南充" },
    { "leshan", "乐山" },
    { "yibin", "宜宾" },
    { "zunyi", "遵义" },
    { "liupanshui", "六盘水" },
    { "qujing", "曲靖" },
    { "baoshan", "保山" },
    { "hanzhong", "汉中" },
    { "baoji", "宝鸡" },
    { "xianyang", "咸阳" },
    { "yanan", "延安" },
    { "yulin", "榆林" },
    { "jiuquan", "酒泉" },
    { "tianshui", "天水" },
    { "hami", "哈密" },
    { "shihezi", "石河子" },
    { "atushi", "阿图什" },

    /* 重名：请在心知 search.json 核对后启用正确 id */
    /* { "shaanxi yulin", "陕西榆林" }, */
    /* { "shanxi yulin", "山西榆林" }, */

    { "sanya", "三亚" },
    { "lianyungang", "连云港" },
    { "yancheng", "盐城" },
    { "huaian", "淮安" },
    { "zhenjiang", "镇江" },
    /* 泰州与浙江台州均可能映射为 taizhou，二选一请用心知 search 区分，例如 jiangsu taizhou */
    /* { "jiangsu taizhou", "泰州" }, */
    { "suqian", "宿迁" },
};

size_t weather_city_get_count(void)
{
    return sizeof(kWeatherCities) / sizeof(kWeatherCities[0]);
}

const weather_city_entry_t *weather_city_get_entry(size_t index)
{
    size_t n = weather_city_get_count();
    if (index >= n) {
        return NULL;
    }
    return &kWeatherCities[index];
}

bool weather_city_is_valid_id(const char *id)
{
    if (!id || !id[0]) {
        return false;
    }
    size_t n = weather_city_get_count();
    for (size_t i = 0; i < n; i++) {
        /* NVS/手工输入可能大小写不一致，心知 location 为小写拼音 */
        if (strcasecmp(kWeatherCities[i].id, id) == 0) {
            return true;
        }
    }
    return false;
}

const char *weather_city_get_name_zh(const char *id)
{
    if (!id || !id[0]) {
        return NULL;
    }
    size_t n = weather_city_get_count();
    for (size_t i = 0; i < n; i++) {
        if (strcmp(kWeatherCities[i].id, id) == 0) {
            return kWeatherCities[i].name_zh;
        }
    }
    return NULL;
}

void weather_city_format_title_case(const char *id, char *out, size_t out_sz)
{
    if (!out || out_sz == 0) {
        return;
    }
    out[0] = '\0';
    if (!id || !id[0]) {
        return;
    }

    size_t j = 0;
    bool word_start = true;
    for (size_t i = 0; id[i] != '\0' && j < out_sz - 1; i++) {
        unsigned char c = (unsigned char)id[i];
        if (c == ' ' || c == '_') {
            out[j++] = (char)(c == '_' ? ' ' : c);
            word_start = true;
            continue;
        }
        if (word_start) {
            if (c >= 'a' && c <= 'z') {
                c = (unsigned char)(c - 'a' + 'A');
            }
            word_start = false;
        } else {
            if (c >= 'A' && c <= 'Z') {
                c = (unsigned char)(c - 'A' + 'a');
            }
        }
        out[j++] = (char)c;
    }
    out[j] = '\0';
}
