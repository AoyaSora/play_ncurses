#include <stdio.h>
#include <time.h>

int main()
{
	time_t t = time(NULL);
	struct tm *local = localtime(&t);

	char buf[128];
	    strftime(buf, sizeof(buf),"%Y/%m/%d",local);//%H:%M:%S %A


	printf("%s", buf);
	// getchar();
}

// 時間の取得はできるが，これを使ってDBを操作する場合，数値として値を比較できた方がいいのでは
// 送信したい情報はAppContentの中にobjectを格納している．dateはcharなのでそのままの形でok