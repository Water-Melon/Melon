/*
 * Copyright (C) Niklaus F.Schen.
 */
sys = Import('sys');
if (MASTER)
    sys.print('master process');
else
    sys.print('worker process');

Pipe('subscribe');
while (1) {
    ret = Pipe('recv');
    if (ret) {
        for (i = 0; i < sys.size(ret); ++i) {
            sys.print(ret[i]);
        }
    } fi
    sys.msleep(1000);
}
Pipe('unsubscribe');
