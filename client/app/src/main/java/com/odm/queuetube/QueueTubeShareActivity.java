package com.odm.queuetube;

import android.content.Intent;
import android.graphics.drawable.Drawable;
import android.support.graphics.drawable.Animatable2Compat;
import android.support.v7.app.AppCompatActivity;
import android.os.Bundle;

    import android.support.graphics.drawable.AnimatedVectorDrawableCompat;
import android.widget.ImageView;
import android.widget.TextView;

public class QueueTubeShareActivity extends AppCompatActivity {

    private AnimatedVectorDrawableCompat anim_wait = null;
    private AnimatedVectorDrawableCompat anim_check = null;
    private AnimatedVectorDrawableCompat anim_cross= null;

    class QueueTubeAnimationCallback extends Animatable2Compat.AnimationCallback
    {
        QueueTubeShareActivity activity = null;
        public QueueTubeAnimationCallback(QueueTubeShareActivity act)
        {
            activity = act;
        }

        @Override
        public void onAnimationEnd(Drawable drawable)
        {
            android.os.SystemClock.sleep(1000);
            activity.finishAndRemoveTask();
        }
    }

    @Override
    protected void onCreate(Bundle savedInstanceState) {
        super.onCreate(savedInstanceState);
        QueueTubeAnimationCallback cb = new QueueTubeAnimationCallback(this);

        setContentView(R.layout.activity_queue_tube_share);
        Bundle extras = getIntent().getExtras();
        String urlstring = extras.getString(Intent.EXTRA_TEXT);
        anim_wait = AnimatedVectorDrawableCompat.create(this, R.drawable.wait);
        anim_cross = AnimatedVectorDrawableCompat.create(this, R.drawable.cross);
        anim_cross.registerAnimationCallback(cb);
        anim_check = AnimatedVectorDrawableCompat.create(this, R.drawable.check);
        anim_check.registerAnimationCallback(cb);

        ImageView img_view = findViewById(R.id.image_id);
        img_view.setImageDrawable(anim_wait);
        anim_wait.start();
        new PostPageTask().execute(new PostPageTaskConfig(this, urlstring));
    }

    public void onComplete(int result)
    {
        ImageView img_view = findViewById(R.id.image_id);
        TextView txt = findViewById(R.id.textView);

        if (result == 200)
        {
            img_view.setImageDrawable(anim_check);
            anim_check.start();
            txt.setText(R.string.success);
        }
        else
        {
            img_view.setImageDrawable(anim_cross);

            if (result == 412)
                // no memory or other resources
                txt.setText(R.string.err_resource);
            else if(result == 503)
                // MPD server error
                txt.setText(R.string.err_mpd);
            else
                // Unknown error
                txt.setText(R.string.err_unknown);

            anim_cross.start();
        }
    }
}
