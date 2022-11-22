/*
 * Copyright (C) Niklaus F.Schen.
 */
sys = import('sys');

pipe('subscribe');
while (1) {
    ret = Pipe('recv');
    if (ret) {
        for (i = 0; i < sys.size(ret); ++i) {
            sys.print(ret[i]);
        }
    } fi
    sys.msleep(1000);
}
pipe('unsubscribe');
