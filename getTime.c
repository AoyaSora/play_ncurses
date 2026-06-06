#include <stdio.h>
#include <time.h>

int main() {
    time_t timer;
    struct tm *local_time;
    int year, month, day;

    // 1. 現在時刻（協定世界時からの経過秒数）を取得
    time(&timer);

    // 2. ローカル時刻（現地時間）の構造体に変換
    local_time = localtime(&timer);
	local_time->tm_mday += 1000;
	mktime(local_time);
    // 3. 各要素を数値として抽出
    year = local_time->tm_year + 1900; // tm_yearは1900年からの経過年数
    month = local_time->tm_mon + 1;    // tm_monは0～11で取得される
    day = local_time->tm_mday;         // tm_mdayはそのまま日を表す

	
    // 結果の表示
    printf("年: %d\n", year);
    printf("月: %d\n", month);
    printf("日: %d\n", day);

    return 0;
}

// 時間の取得はできるが，これを使ってDBを操作する場合，数値として値を比較できた方がいいのでは
// 送信したい情報はAppContentの中にobjectを格納している．dateはcharなのでそのままの形でok