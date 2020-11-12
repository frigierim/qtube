package com.odm.queuetube;

import android.app.Activity;

public class PostPageTaskConfig
{
    enum POST_ACTION
    { SEND_URL, RESET };

    Activity context;
    String  url;
    POST_ACTION action;
    PostPageTaskConfig(Activity ctx, String url, POST_ACTION action) { this.context = ctx; this.url = url; this.action = action;}
}
