package com.odm.queuetube;

import android.content.Context;
import android.os.AsyncTask;
import android.preference.PreferenceManager;

import java.io.BufferedReader;
import java.io.IOException;
import java.io.InputStreamReader;
import java.net.HttpURLConnection;
import java.net.MalformedURLException;
import java.net.URL;

public class PostPageTask extends AsyncTask<PostPageTaskConfig, Void, PostPageTask.QueueServerResponse> {

    PostPageTaskConfig cfg;

    public class QueueServerResponse
    {
        int qsr_code;
        String qsr_string;

        QueueServerResponse(int code, String string)
        {
            qsr_code = code;
            qsr_string = string;
        }
    }

    @Override
    protected QueueServerResponse doInBackground(PostPageTaskConfig... configs) {
        cfg = configs[0];
        HttpURLConnection urlConnection = null;
        int resCode = 0;
        String resBody = "";

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

            BufferedReader br;
            String rb;
            if (100 <= resCode && resCode <= 399)
            {
                br = new BufferedReader(new InputStreamReader(urlConnection.getInputStream()));
            }
            else
            {
                br = new BufferedReader(new InputStreamReader(urlConnection.getErrorStream()));
            }
            
            while((rb = br.readLine()) != null)
            {
                resBody += rb;
            }

        } catch (MalformedURLException e) {
            e.printStackTrace();
            resBody = "Malformed URL!";
        } catch (IOException e) {
            resBody = "Server is unreachable!";
        }
        catch (Exception e)
        {
            resBody = "Generic exception!";
            e.printStackTrace();
        }
        finally {
            if (urlConnection != null)
                urlConnection.disconnect();
        }
        return new QueueServerResponse(resCode, resBody);
    }

    protected void onPostExecute(QueueServerResponse result) {
        // Terminate instance
        QueueTubeShareActivity act = (QueueTubeShareActivity)cfg.context;
        act.onComplete(result);
    }
}
