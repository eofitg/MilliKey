#include <ApplicationServices/ApplicationServices.h>
#include <time.h>
#include <stdio.h>
#include <stdbool.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <libgen.h>
#include <limits.h>
#include <mach-o/dyld.h> // _NSGetExecutablePath

// 获取当前时间（毫秒）
uint64_t now_ms() {
    struct timespec ts;
    clock_gettime(CLOCK_REALTIME, &ts);
    return (uint64_t)ts.tv_sec * 1000 + ts.tv_nsec / 1000000;
}

// 忙等到目标毫秒时间
void busy_wait_ms(uint64_t target_ms) {
    while (now_ms() < target_ms) {
        usleep(100);
    }
}

// 从 JSON 文件读取指定字段的整数值
bool parse_json_value(const char* data, const char* key, int* out) {
    char* pos = strstr(data, key);
    if (!pos) return false;
    pos = strchr(pos, ':');
    if (!pos) return false;
    pos++; // 跳过 ':'
    while (*pos == ' ' || *pos == '\t') pos++; // 跳过空格
    char* end_ptr;
    long val = strtol(pos, &end_ptr, 10);
    if (end_ptr == pos) return false;
    *out = (int)val;
    return true;
}

// 读取 JSON 配置文件
bool load_json_simple(const char* filename, int* year, int* month, int* day,
                      int* hour, int* minute, int* second, int* millisecond) {
    FILE* f = fopen(filename, "r");
    if (!f) return false;

    fseek(f, 0, SEEK_END);
    long length = ftell(f);
    fseek(f, 0, SEEK_SET);

    char* data = (char*)malloc(length + 1);
    fread(data, 1, length, f);
    data[length] = '\0';
    fclose(f);

    bool ok = parse_json_value(data, "\"year\"", year)
              && parse_json_value(data, "\"month\"", month)
              && parse_json_value(data, "\"day\"", day)
              && parse_json_value(data, "\"hour\"", hour)
              && parse_json_value(data, "\"minute\"", minute)
              && parse_json_value(data, "\"second\"", second)
              && parse_json_value(data, "\"millisecond\"", millisecond);

    free(data);
    return ok;
}

// 自动生成默认 JSON 配置
void create_default_json(const char* filename) {
    FILE* f = fopen(filename, "w");
    if (!f) {
        printf("无法创建默认 JSON 文件！\n");
        return;
    }
    fprintf(f,
            "{\n"
            "  \"year\": 2025,\n"
            "  \"month\": 9,\n"
            "  \"day\": 20,\n"
            "  \"hour\": 9,\n"
            "  \"minute\": 46,\n"
            "  \"second\": 0,\n"
            "  \"millisecond\": 100\n"
            "}\n");
    fclose(f);
    printf("默认 JSON 文件已生成: %s\n请重新运行程序！\n", filename);
}

void send_cmd_v_enter() {
    int interval_1 = 100;
    int interval_2 = 1200;
    CGEventSourceRef source = CGEventSourceCreate(kCGEventSourceStateCombinedSessionState);

    // 1. Command down
    CGEventRef cmdDown = CGEventCreateKeyboardEvent(source, (CGKeyCode)55, true);
    CGEventPost(kCGHIDEventTap, cmdDown);
    usleep(interval_1);

    // 2. V down with Command flag
    CGEventRef vDown = CGEventCreateKeyboardEvent(source, (CGKeyCode)9, true);
    CGEventSetFlags(vDown, kCGEventFlagMaskCommand);
    CGEventPost(kCGHIDEventTap, vDown);
    usleep(interval_1);

    // 3. V up with Command flag
    CGEventRef vUp = CGEventCreateKeyboardEvent(source, (CGKeyCode)9, false);
    CGEventSetFlags(vUp, kCGEventFlagMaskCommand);
    CGEventPost(kCGHIDEventTap, vUp);
    usleep(interval_1);

    // 4. Command up
    CGEventRef cmdUp = CGEventCreateKeyboardEvent(source, (CGKeyCode)55, false);
    CGEventPost(kCGHIDEventTap, cmdUp);
    usleep(interval_2);

    // 5. Enter down
    CGEventRef enterDown = CGEventCreateKeyboardEvent(source, (CGKeyCode)36, true);
    CGEventPost(kCGHIDEventTap, enterDown);
    usleep(interval_1);

    // 6. Enter up
    CGEventRef enterUp = CGEventCreateKeyboardEvent(source, (CGKeyCode)36, false);
    CGEventPost(kCGHIDEventTap, enterUp);

    CFRelease(cmdDown);
    CFRelease(vDown);
    CFRelease(vUp);
    CFRelease(cmdUp);
    CFRelease(enterDown);
    CFRelease(enterUp);
    CFRelease(source);
}

int main() {
    // 获取可执行文件所在目录
    char exePath[PATH_MAX];
    uint32_t size = sizeof(exePath);
    if (_NSGetExecutablePath(exePath, &size) != 0) {
        printf("获取可执行文件路径失败!\n");
        return 1;
    }
    char* dir = dirname(exePath);

    char json_file[PATH_MAX];
    snprintf(json_file, sizeof(json_file), "%s/config.json", dir);

    int year, month, day, hour, minute, second, millisecond;

    if (!load_json_simple(json_file, &year, &month, &day, &hour, &minute, &second, &millisecond)) {
        printf("找不到 JSON 配置文件，正在生成默认文件...\n");
        create_default_json(json_file);
        // year = 2025; month = 9; day = 20;
        // hour = 9; minute = 46; second = 0; millisecond = 100;
        return 0;
    }

    struct tm target_tm = {0};
    target_tm.tm_year = year - 1900;
    target_tm.tm_mon  = month - 1;
    target_tm.tm_mday = day;
    target_tm.tm_hour = hour;
    target_tm.tm_min  = minute;
    target_tm.tm_sec  = second;

    time_t target_time = mktime(&target_tm);
    uint64_t target_ms = (uint64_t)target_time * 1000 + millisecond;

    printf("等待检测时间点 %04d-%02d-%02d %02d:%02d:%02d.%03d...\n",
           year, month, day, hour, minute, second, millisecond);

    bool triggered = false;
    while (!triggered) {
        uint64_t now = now_ms();
        int64_t diff = (int64_t)(target_ms - now);

        if (diff <= 0) {
            send_cmd_v_enter();
            printf("触发时间点! 当前时间戳(ms)=%llu\n", now);
            triggered = true;
        } else if (diff > 60000) {
            sleep(60);
        } else if (diff > 10000) {
            sleep(10);
        } else {
            busy_wait_ms(target_ms);
        }
    }

    return 0;
}
