package com.odm.queuetube;

import android.app.Activity;

public class PostPageTaskConfig
{
    Activity context;
    String  url;
    PostPageTaskConfig(Activity ctx, String url) { this.context = ctx; this.url = url;}
}
