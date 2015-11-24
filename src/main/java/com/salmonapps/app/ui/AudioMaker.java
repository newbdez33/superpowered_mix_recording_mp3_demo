package com.podbean.app.podcast.ui;

import android.app.AlertDialog;
import android.content.Context;
import android.content.DialogInterface;
import android.content.Intent;
import android.content.res.AssetFileDescriptor;
import android.media.AudioManager;
import android.media.MediaPlayer;
import android.os.Build;
import android.os.Bundle;
import android.view.View;
import android.widget.Button;
import android.widget.CompoundButton;
import android.widget.SeekBar;
import android.widget.TextView;
import android.widget.ToggleButton;

import com.lidroid.xutils.util.LogUtils;
import com.podbean.app.podcast.R;
import com.podbean.app.podcast.utils.RecorderUtil;
import com.podbean.app.podcast.utils.UITools;

import java.io.File;
import java.io.IOException;
import java.util.Timer;
import java.util.TimerTask;

public class AudioMaker extends BaseActivity {

    //ui outlet
    SeekBar seekBar;
    ToggleButton recordToggleButton;
    ToggleButton playbackButton;
    Button remakeButton;
    Button saveButton;
    TextView timeTextView;

    Timer timer;

    //
    private String podcastId;

    //path for recordings
    private String recordingPath;

    private MediaPlayer mediaPlayer;

    private boolean isRecording;

    @Override
    protected void onCreate(Bundle savedInstanceState) {
        super.onCreate(savedInstanceState);

        podcastId = getIntent().getStringExtra("podcastId");
        if(null == podcastId) {
            UITools.toast("Invalid podcast id.", this);
            return;
        }

        setContentView(R.layout.activity_audio_maker);

        //prepare ui
        remakeButton = (Button)findViewById(R.id.remakeButton);
        saveButton = (Button)findViewById(R.id.saveButton);
        timeTextView = (TextView)findViewById(R.id.textView_time);
        recordToggleButton = (ToggleButton)findViewById(R.id.toggleButton_Record);
        recordToggleButton.setOnCheckedChangeListener(new CompoundButton.OnCheckedChangeListener() {
            @Override
            public void onCheckedChanged(CompoundButton compoundButton, boolean b) {
                if (b) {
                    AudioMaker.this.onRecord();
                }else {
                    AudioMaker.this.onStopRecord();
                }
            }
        });

        playbackButton = (ToggleButton)findViewById(R.id.toggleButton_Play);
        playbackButton.setOnCheckedChangeListener(new CompoundButton.OnCheckedChangeListener() {
            @Override
            public void onCheckedChanged(CompoundButton compoundButton, boolean b) {
                if (b) {
                    AudioMaker.this.onPlayBack();
                } else {
                    AudioMaker.this.onStopPlayBack();
                }
            }
        });

        seekBar = (SeekBar)findViewById(R.id.seekBar);
        seekBar.setOnSeekBarChangeListener(new SeekBar.OnSeekBarChangeListener() {
            @Override
            public void onProgressChanged(SeekBar seekBar, int i, boolean b) {
                RecorderUtil jni = RecorderUtil.getInstance();
                jni.onVolume(i);
            }

            @Override
            public void onStartTrackingTouch(SeekBar seekBar) {

            }

            @Override
            public void onStopTrackingTouch(SeekBar seekBar) {

            }
        });

        //prepare dir
        String dirpath = UITools.getMyPdcDir() + "/recording";
        File saveDir = new File(dirpath);
        if (!saveDir.isDirectory()) {
            saveDir.mkdir();
        }
        recordingPath = dirpath+"/recording.mp3";
        File file = new File(recordingPath);
        file.delete();  //remove recording file at init



        isRecording = false;
        setupRecorderInstance();
    }

    private void updateTimeTextView() {

        if (this.mediaPlayer != null && this.mediaPlayer.isPlaying()) {
            int cur = mediaPlayer.getCurrentPosition();
//            int dur = mediaPlayer.getDuration();
            int cm = cur/1000 / 60;
            int cs = cur/1000 % 60;
//            int dm = dur/60;
//            int ds = dm % 60;
            this.timeTextView.setText(String.format("%02d:%02d", cm, cs));
        }
    }

    private void setupRecorderInstance() {
        ////////
        // Get the device's sample rate and buffer size to enable low-latency Android audio output, if available.
        String samplerateString = null, buffersizeString = null;
        if (Build.VERSION.SDK_INT >= 17) {
            AudioManager audioManager = (AudioManager) this.getSystemService(Context.AUDIO_SERVICE);
            samplerateString = audioManager.getProperty(AudioManager.PROPERTY_OUTPUT_SAMPLE_RATE);
            buffersizeString = audioManager.getProperty(AudioManager.PROPERTY_OUTPUT_FRAMES_PER_BUFFER);
        }
        if (samplerateString == null) samplerateString = "44100";
        if (buffersizeString == null) buffersizeString = "512";

        // Files under res/raw are not compressed, just copied into the APK. Get the offset and length to know where our files are located.
        AssetFileDescriptor fd0 = getResources().openRawResourceFd(R.raw.country);
        long[] params = {
                fd0.getStartOffset(),
                fd0.getLength(),
                fd0.getStartOffset(),
                fd0.getLength(),
                Integer.parseInt(samplerateString),
                Integer.parseInt(buffersizeString)
        };
        try {
            fd0.getParcelFileDescriptor().close();
        } catch (IOException e) {
            android.util.Log.d("", "Close error.");
        }

        RecorderUtil jni = RecorderUtil.getInstance();
        LogUtils.i(String.format("JNI loaded:%s", jni.getCLanguageString()));
        jni.PodbeanRecorder(getPackageResourcePath(), params, recordingPath);

    }

    public void onRecord() {

        if ( isRecording ) return;

        playbackButton.setChecked(false);
        if (mediaPlayer != null) {
            mediaPlayer.stop();
            mediaPlayer = null;
        }
        isRecording = true;
        RecorderUtil jni = RecorderUtil.getInstance();
        jni.onRecord(true);
    }

    public void onStopRecord() {

        if (!isRecording) return;

        isRecording = false;
        RecorderUtil jni = RecorderUtil.getInstance();
        jni.onRecord(false);

        remakeButton.setVisibility(View.VISIBLE);
        saveButton.setVisibility(View.VISIBLE);
    }

    public void onGoodtime(View view) {

        loadBackgroundMusic(R.raw.goodtime);

    }

    public void onCountry(View view) {

        loadBackgroundMusic(R.raw.country);

    }

    public void onDivision(View view) {

        loadBackgroundMusic(R.raw.division);

    }

    public void onBlank(View view) {
        loadBackgroundMusic(R.raw.blank);
    }

    private void loadBackgroundMusic(int id) {

        AssetFileDescriptor fd0 = getResources().openRawResourceFd(id);
        long[] params = {
                fd0.getStartOffset(),
                fd0.getLength(),
                fd0.getStartOffset(),
                fd0.getLength()
        };
        try {
            fd0.getParcelFileDescriptor().close();
        } catch (IOException e) {
            android.util.Log.d("", "Close error.");
        }

        RecorderUtil jni = RecorderUtil.getInstance();
        jni.onMusicSelect(getPackageResourcePath(), params);
    }

    public void onStopPlayBack() {
        timer.cancel();
        if (mediaPlayer == null) return;
        mediaPlayer.stop();
        mediaPlayer = null;
    }

    public void onPlayBack() {

        RecorderUtil jni = RecorderUtil.getInstance();
        if (isRecording) jni.onRecord(false);
        recordToggleButton.setChecked(false);
        jni.onPlayBackgroundMusic(false);

        if (mediaPlayer!=null) {
            mediaPlayer.stop();
            mediaPlayer = null;
        }

        mediaPlayer = new MediaPlayer();
        try {
            mediaPlayer.setDataSource(recordingPath);
            mediaPlayer.prepare();
            mediaPlayer.start();
        }catch (Exception e) {
            android.util.Log.d("TESTX", "IO error.");
            AudioMaker.this.playbackButton.setChecked(false);
            return;
        }

        mediaPlayer.setOnCompletionListener(new MediaPlayer.OnCompletionListener() {
            @Override
            public void onCompletion(MediaPlayer mediaPlayer) {
                AudioMaker.this.playbackButton.setChecked(false);
                timer.cancel();
            }
        });

        timer = new Timer();
        timer.scheduleAtFixedRate(new TimerTask() {
            @Override
            public void run() {
                runOnUiThread(new Runnable() {
                    @Override
                    public void run() {
                        updateTimeTextView();
                    }
                });
            }
        }, 0, 1000);

    }

    private AlertDialog remakeDialog = null;

    private void dismissRemakeDialog() {

        if (remakeDialog != null) {
            remakeDialog.dismiss();
            remakeDialog = null;
        }
    }

    public void onRemake(View view) {

        dismissRemakeDialog();

        String title = "Confirm";
        String message = "The recorded audio will be deleted. Are you sure to remake?";

        if (remakeDialog == null) {
            remakeDialog = new AlertDialog.Builder(this)
                    .setIcon(android.R.drawable.ic_dialog_info)
                    .setTitle(title)
                    .setMessage(message)
                    .setNegativeButton("No", new DialogInterface.OnClickListener() {
                        @Override
                        public void onClick(DialogInterface dialog, int which) {
                            //
                        }
                    })
                    .setOnCancelListener(new DialogInterface.OnCancelListener() {
                        @Override
                        public void onCancel(DialogInterface dialog) {
                            //
                        }
                    })
                    .setPositiveButton("Yes", new DialogInterface.OnClickListener() {
                        @Override
                        public void onClick(DialogInterface dialog, int which) {
                            AudioMaker.this.doRemake();
                        }
                    })
                    .create();


        } else {
            remakeDialog.setMessage(message);
        }

        remakeDialog.setCancelable(true);
        remakeDialog.show();
    }

    private void doRemake() {
        playbackButton.setChecked(false);
        if (mediaPlayer != null) {
            mediaPlayer.stop();
            mediaPlayer = null;
        }
        if (isRecording) {
            onStopRecord();
        }
        remakeButton.setVisibility(View.INVISIBLE);
        saveButton.setVisibility(View.INVISIBLE);

        RecorderUtil jni = RecorderUtil.getInstance();
        jni.offline();

        //remove exists file
        File file = new File(recordingPath);
        file.delete();  //remove recording file at init

        setupRecorderInstance();
    }

    public void onSave(View view) {

        playbackButton.setChecked(false);
        if (mediaPlayer != null) {
            mediaPlayer.stop();
            mediaPlayer = null;
        }

        Intent intent = new Intent(this, NewEpisodeActivity.class);
        intent.putExtra("podcastId", podcastId);
        intent.putExtra("mode", MyPdcActivity.CREATE_MODE);
        intent.putExtra("media_path", recordingPath);
        startActivityForResult(intent, MyPdcActivity.REQUEST_NEW_EPI);
    }

    @Override
    protected void onActivityResult(int requestCode, int resultCode, Intent data) {

        super.onActivityResult(requestCode, resultCode, data);

        if(MyPdcActivity.REQUEST_NEW_EPI == requestCode) {
            if(MyPdcActivity.RESULT_CODE_SAVE_DRAFT == resultCode) {
                setResult(MyPdcActivity.RESULT_CODE_SAVE_DRAFT);
                finish();
            } else if(MyPdcActivity.RESULT_CODE_PUBLISH == resultCode){
                setResult(MyPdcActivity.RESULT_CODE_PUBLISH);
                finish();
            }
        }
    }

    @Override
    protected void onStop() {
        super.onStop();
        RecorderUtil jni = RecorderUtil.getInstance();
        jni.onPlayBackgroundMusic(false);
        jni.onRecord(false);

        jni.offline();
    }
}
