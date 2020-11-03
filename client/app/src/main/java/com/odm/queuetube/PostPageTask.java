package com.odm.queuetube;

import android.content.Context;
import android.os.AsyncTask;
import android.preference.PreferenceManager;

import java.io.IOException;
import java.net.HttpURLConnection;
import java.net.MalformedURLException;
import java.net.URL;

public class PostPageTask extends AsyncTask<PostPageTaskConfig, Void, Integer> {

    PostPageTaskConfig cfg;

    @Override
    protected Integer doInBackground(PostPageTaskConfig... configs) {
        cfg = configs[0];
        HttpURLConnection urlConnection = null;
        int resCode = 0;
        try {
            String host = PreferenceManager
                    .getDefaultSharedPreferences(configs[0].context)
                    .getString("address", "");

            String port= PreferenceManager
                    .getDefaultSharedPreferences(configs[0].context)
                    .getString("port", "");

            URL url = new URL("http", host, Integer.valueOf(port), "add?url=" + configs[0].url);
            urlConnection = (HttpURLConnection) url.openConnection();
            urlConnection.setRequestMethod("GET");
            urlConnection.setConnectTimeout(20000);
            urlConnection.connect();
            resCode = urlConnection.getResponseCode();

        } catch (MalformedURLException e) {
            e.printStackTrace();
        } catch (IOException e) {
            // Error reported by service
        }
        catch (Exception e)
        {
            e.printStackTrace();
        }
        finally {
            if (urlConnection != null)
                urlConnection.disconnect();
        }
        return resCode;
    }

    protected void onPostExecute(Integer result) {
        // Terminate instance
        QueueTubeShareActivity act = (QueueTubeShareActivity)cfg.context;
        act.onComplete(result.intValue());
    }
}
