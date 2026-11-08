package com.sensipro.app;

import android.app.Notification;
import android.app.NotificationChannel;
import android.app.NotificationManager;
import android.app.PendingIntent;
import android.app.Service;
import android.content.Context;
import android.content.Intent;
import android.content.SharedPreferences;
import android.graphics.Canvas;
import android.graphics.Color;
import android.graphics.Paint;
import android.graphics.PixelFormat;
import android.graphics.RectF;
import android.os.Build;
import android.os.IBinder;
import android.provider.Settings;
import android.view.Gravity;
import android.view.MotionEvent;
import android.view.View;
import android.view.ViewGroup;
import android.view.WindowManager;
import android.widget.Button;
import android.widget.FrameLayout;
import android.widget.LinearLayout;
import android.widget.TextView;
import android.widget.Toast;
import org.json.JSONObject;

public class AimHudService extends Service {

    public static final String ACTION_START = "com.sensipro.app.action.START_AIMHUD";
    public static final String ACTION_STOP = "com.sensipro.app.action.STOP_AIMHUD";
    public static final String ACTION_UPDATE = "com.sensipro.app.action.UPDATE_AIMHUD";
    public static final String ACTION_LOCK = "com.sensipro.app.action.LOCK_AIMHUD";
    public static final String ACTION_UNLOCK = "com.sensipro.app.action.UNLOCK_AIMHUD";
    public static final String ACTION_RESET_POS = "com.sensipro.app.action.RESET_AIMHUD_POS";
    public static final String ACTION_APPLY_MACRO = "com.sensipro.app.action.APPLY_MACRO";
    public static final String ACTION_REMOVE_MACRO = "com.sensipro.app.action.REMOVE_MACRO";
    public static final String ACTION_LOCK_MACRO = "com.sensipro.app.action.LOCK_MACRO";
    public static final String ACTION_UNLOCK_MACRO = "com.sensipro.app.action.UNLOCK_MACRO";
    public static final String ACTION_RESET_MACRO = "com.sensipro.app.action.RESET_MACRO";

    public static final String EXTRA_CONFIG_JSON = "config_json";
    public static final String CHANNEL_ID = "sensipro_aimhud_channel";
    public static final int NOTIFICATION_ID = 2001;

    public static volatile boolean isRunning = false;
    public static volatile boolean isLocked = true;
    public static volatile boolean isMacroLocked = true;

    private WindowManager mWindowManager;
    private FrameLayout mRootContainer;
    private AimReticleView mReticleView;
    private MacroLayoutView mMacroView;
    private LinearLayout mEditBar;
    private WindowManager.LayoutParams mParams;

    // Dedicated Macro Overlay Container & View
    private FrameLayout mMacroContainer;
    private MacroLayoutView mDedicatedMacroView;
    private WindowManager.LayoutParams mMacroParams;

    // Config state
    private boolean mReticleEnabled = true;
    private String mStyle = "plus"; // circle, plus, cross, dot, ring
    private int mSizeDp = 36;
    private float mOpacity = 0.85f;
    private float mRotation = 0f;
    private int mColor = Color.parseColor("#00E5FF");
    private int mPosX = 0;
    private int mPosY = 0;

    // Macro config state
    private boolean mShowMacroGuides = false;
    private int mMacroFireSize = 48;
    private float mMacroOpacity = 0.85f;
    private int mMacroColor = Color.parseColor("#00E676");
    private int mMacroFireX = 140;
    private int mMacroFireY = 160;

    @Override
    public IBinder onBind(Intent intent) {
        return null;
    }

    @Override
    public void onCreate() {
        super.onCreate();
        mWindowManager = (WindowManager) getSystemService(WINDOW_SERVICE);
        createNotificationChannel();
        loadSavedConfig();
    }

    @Override
    public int onStartCommand(Intent intent, int flags, int startId) {
        if (intent != null) {
            String action = intent.getAction();
            if (ACTION_STOP.equals(action)) {
                stopSelf();
                return START_NOT_STICKY;
            } else if (ACTION_LOCK.equals(action)) {
                setLockedState(true);
                return START_STICKY;
            } else if (ACTION_UNLOCK.equals(action)) {
                setLockedState(false);
                return START_STICKY;
            } else if (ACTION_RESET_POS.equals(action)) {
                resetPosition();
                return START_STICKY;
            } else if (ACTION_LOCK_MACRO.equals(action)) {
                setMacroLockedState(true);
                return START_STICKY;
            } else if (ACTION_UNLOCK_MACRO.equals(action)) {
                setMacroLockedState(false);
                return START_STICKY;
            } else if (ACTION_RESET_MACRO.equals(action)) {
                resetMacroPosition();
                return START_STICKY;
            } else if (ACTION_REMOVE_MACRO.equals(action)) {
                removeMacroOverlayDirect();
                return START_STICKY;
            } else if (ACTION_APPLY_MACRO.equals(action)) {
                mShowMacroGuides = true;
                if (intent.hasExtra(EXTRA_CONFIG_JSON)) {
                    parseConfigJson(intent.getStringExtra(EXTRA_CONFIG_JSON));
                }
            } else if (ACTION_UPDATE.equals(action)) {
                if (intent.hasExtra(EXTRA_CONFIG_JSON)) {
                    parseConfigJson(intent.getStringExtra(EXTRA_CONFIG_JSON));
                }
                updateOverlayAppearance();
                return START_STICKY;
            } else if (ACTION_START.equals(action)) {
                if (intent.hasExtra(EXTRA_CONFIG_JSON)) {
                    parseConfigJson(intent.getStringExtra(EXTRA_CONFIG_JSON));
                }
            }
        }

        // Verify overlay permission
        if (Build.VERSION.SDK_INT >= Build.VERSION_CODES.M && !Settings.canDrawOverlays(this)) {
            stopSelf();
            return START_NOT_STICKY;
        }

        startForegroundNotification();

        if (mReticleEnabled && mRootContainer == null) {
            initOverlayView();
        }

        if (mShowMacroGuides) {
            if (mMacroContainer == null) {
                initMacroOverlayView();
            } else {
                updateMacroOverlayAppearance();
            }
        }

        updateOverlayAppearance();
        isRunning = true;
        return START_STICKY;
    }

    private void startForegroundNotification() {
        Intent notificationIntent = new Intent(this, MainActivity.class);
        PendingIntent pendingIntent = PendingIntent.getActivity(
                this, 0, notificationIntent,
                Build.VERSION.SDK_INT >= Build.VERSION_CODES.M ? PendingIntent.FLAG_IMMUTABLE : 0
        );

        Intent exitIntent = new Intent(this, AimHudService.class);
        exitIntent.setAction(ACTION_STOP);
        PendingIntent exitPendingIntent = PendingIntent.getService(
                this, 1, exitIntent,
                Build.VERSION.SDK_INT >= Build.VERSION_CODES.M ? PendingIntent.FLAG_IMMUTABLE : 0
        );

        Intent lockIntent = new Intent(this, AimHudService.class);
        lockIntent.setAction(isLocked ? ACTION_UNLOCK : ACTION_LOCK);
        PendingIntent lockPendingIntent = PendingIntent.getService(
                this, 2, lockIntent,
                Build.VERSION.SDK_INT >= Build.VERSION_CODES.M ? PendingIntent.FLAG_UPDATE_CURRENT | PendingIntent.FLAG_IMMUTABLE : PendingIntent.FLAG_UPDATE_CURRENT
        );

        String statusText = isLocked ? "AimHUD Active • Locked (Pass-through Touches)" : "AimHUD Active • Drag Mode (Repositioning)";
        String lockActionTitle = isLocked ? "Reposition / Unlock" : "Lock Position";

        Notification.Builder builder;
        if (Build.VERSION.SDK_INT >= Build.VERSION_CODES.O) {
            builder = new Notification.Builder(this, CHANNEL_ID);
        } else {
            builder = new Notification.Builder(this);
        }

        builder.setContentTitle("SENSI PRO — Floating AimHUD")
                .setContentText(statusText)
                .setSmallIcon(R.mipmap.ic_launcher)
                .setContentIntent(pendingIntent)
                .setOngoing(true)
                .addAction(0, lockActionTitle, lockPendingIntent)
                .addAction(0, "Exit HUD", exitPendingIntent);

        if (Build.VERSION.SDK_INT >= 34) {
            startForeground(NOTIFICATION_ID, builder.build(), android.content.pm.ServiceInfo.FOREGROUND_SERVICE_TYPE_SPECIAL_USE);
        } else {
            startForeground(NOTIFICATION_ID, builder.build());
        }
    }

    private void createNotificationChannel() {
        if (Build.VERSION.SDK_INT >= Build.VERSION_CODES.O) {
            NotificationChannel channel = new NotificationChannel(
                    CHANNEL_ID,
                    "SENSI PRO AimHUD Overlay",
                    NotificationManager.IMPORTANCE_LOW
            );
            channel.setDescription("Shows active status of the floating AimHUD gaming overlay");
            channel.enableVibration(false);
            channel.enableLights(false);
            NotificationManager nm = (NotificationManager) getSystemService(Context.NOTIFICATION_SERVICE);
            if (nm != null) {
                nm.createNotificationChannel(channel);
            }
        }
    }

    private int calculateWindowWidth() {
        if (mShowMacroGuides) {
            return dp(420);
        }
        if (isLocked) {
            return dp(mSizeDp * 2) + dp(16);
        } else {
            return Math.max(dp(260), dp(mSizeDp * 2) + dp(40));
        }
    }

    private int calculateWindowHeight() {
        if (mShowMacroGuides) {
            return dp(420);
        }
        if (isLocked) {
            return dp(mSizeDp * 2) + dp(16);
        } else {
            return dp(mSizeDp * 2) + dp(120);
        }
    }

    private void initOverlayView() {
        // Prevent duplicate overlay windows
        if (mRootContainer != null && mWindowManager != null) {
            try {
                mWindowManager.removeView(mRootContainer);
            } catch (Exception e) {}
            mRootContainer = null;
        }

        int layoutType;
        if (Build.VERSION.SDK_INT >= Build.VERSION_CODES.O) {
            layoutType = WindowManager.LayoutParams.TYPE_APPLICATION_OVERLAY;
        } else {
            layoutType = WindowManager.LayoutParams.TYPE_PHONE;
        }

        int flags = WindowManager.LayoutParams.FLAG_NOT_FOCUSABLE
                | WindowManager.LayoutParams.FLAG_LAYOUT_NO_LIMITS
                | WindowManager.LayoutParams.FLAG_LAYOUT_IN_SCREEN
                | WindowManager.LayoutParams.FLAG_HARDWARE_ACCELERATED;

        if (isLocked) {
            // In locked mode, FLAG_NOT_TOUCHABLE guarantees 100% pass-through touches to game
            flags |= WindowManager.LayoutParams.FLAG_NOT_TOUCHABLE;
        } else {
            flags |= WindowManager.LayoutParams.FLAG_NOT_TOUCH_MODAL;
        }

        int winWidth = calculateWindowWidth();
        int winHeight = calculateWindowHeight();

        mParams = new WindowManager.LayoutParams(
                winWidth,
                winHeight,
                layoutType,
                flags,
                PixelFormat.TRANSLUCENT
        );

        if (Build.VERSION.SDK_INT >= Build.VERSION_CODES.P) {
            mParams.layoutInDisplayCutoutMode = WindowManager.LayoutParams.LAYOUT_IN_DISPLAY_CUTOUT_MODE_SHORT_EDGES;
        }

        mParams.gravity = Gravity.CENTER;
        mParams.x = mPosX;
        mParams.y = mPosY;

        // Root container holds Reticle, Macro elements, and optional edit bar
        mRootContainer = new FrameLayout(this);
        mRootContainer.setClipChildren(false);
        mRootContainer.setClipToPadding(false);

        // Macro layout guides behind reticle
        mMacroView = new MacroLayoutView(this);
        mRootContainer.addView(mMacroView, new FrameLayout.LayoutParams(
                FrameLayout.LayoutParams.WRAP_CONTENT,
                FrameLayout.LayoutParams.WRAP_CONTENT,
                Gravity.CENTER
        ));

        // Center reticle
        mReticleView = new AimReticleView(this);
        int pxSize = dp(mSizeDp * 2);
        mRootContainer.addView(mReticleView, new FrameLayout.LayoutParams(
                pxSize,
                pxSize,
                Gravity.CENTER
        ));

        // Floating Edit Bar (visible only when UNLOCKED / in Repositioning Mode)
        mEditBar = buildEditBar();
        mRootContainer.addView(mEditBar);
        mEditBar.setVisibility(isLocked ? View.GONE : View.VISIBLE);

        // Drag listener for manual repositioning
        mRootContainer.setOnTouchListener(new View.OnTouchListener() {
            private int initialX, initialY;
            private float initialTouchX, initialTouchY;
            private boolean isMoved = false;

            @Override
            public boolean onTouch(View v, MotionEvent event) {
                if (isLocked) {
                    return false; // Touches pass through
                }

                switch (event.getAction()) {
                    case MotionEvent.ACTION_DOWN:
                        initialX = mParams.x;
                        initialY = mParams.y;
                        initialTouchX = event.getRawX();
                        initialTouchY = event.getRawY();
                        isMoved = false;
                        return true;

                    case MotionEvent.ACTION_MOVE:
                        float dx = event.getRawX() - initialTouchX;
                        float dy = event.getRawY() - initialTouchY;
                        if (Math.abs(dx) > 3 || Math.abs(dy) > 3) {
                            isMoved = true;
                        }
                        mParams.x = initialX + (int) dx;
                        mParams.y = initialY + (int) dy;
                        mPosX = mParams.x;
                        mPosY = mParams.y;
                        try {
                            mWindowManager.updateViewLayout(mRootContainer, mParams);
                        } catch (Exception e) {}
                        return true;

                    case MotionEvent.ACTION_UP:
                        if (isMoved) {
                            savePosition(mPosX, mPosY);
                        }
                        return true;
                }
                return false;
            }
        });

        updateOverlayAppearance();

        try {
            mWindowManager.addView(mRootContainer, mParams);
        } catch (Exception e) {
            mRootContainer = null;
            isRunning = false;
            stopSelf();
        }
    }

    private LinearLayout buildEditBar() {
        LinearLayout bar = new LinearLayout(this);
        bar.setOrientation(LinearLayout.HORIZONTAL);
        bar.setGravity(Gravity.CENTER);
        bar.setBackgroundColor(Color.parseColor("#D90A0D14"));
        bar.setPadding(dp(8), dp(4), dp(8), dp(4));

        FrameLayout.LayoutParams lp = new FrameLayout.LayoutParams(
                FrameLayout.LayoutParams.WRAP_CONTENT,
                FrameLayout.LayoutParams.WRAP_CONTENT,
                Gravity.BOTTOM | Gravity.CENTER_HORIZONTAL
        );
        lp.bottomMargin = dp(-45);
        bar.setLayoutParams(lp);

        // Lock button
        Button btnLock = new Button(this);
        btnLock.setText("🔒 Lock");
        btnLock.setTextSize(10f);
        btnLock.setTextColor(Color.parseColor("#00E676"));
        btnLock.setBackgroundColor(Color.parseColor("#2500E676"));
        btnLock.setPadding(dp(6), dp(2), dp(6), dp(2));
        btnLock.setOnClickListener(new View.OnClickListener() {
            @Override
            public void onClick(View v) {
                setLockedState(true);
            }
        });
        bar.addView(btnLock);

        // Center button
        Button btnCenter = new Button(this);
        btnCenter.setText("⛶ Center");
        btnCenter.setTextSize(10f);
        btnCenter.setTextColor(Color.parseColor("#00E5FF"));
        btnCenter.setBackgroundColor(Color.parseColor("#2500E5FF"));
        btnCenter.setPadding(dp(6), dp(2), dp(6), dp(2));
        LinearLayout.LayoutParams clp = new LinearLayout.LayoutParams(
                LinearLayout.LayoutParams.WRAP_CONTENT,
                LinearLayout.LayoutParams.WRAP_CONTENT
        );
        clp.setMargins(dp(4), 0, dp(4), 0);
        btnCenter.setLayoutParams(clp);
        btnCenter.setOnClickListener(new View.OnClickListener() {
            @Override
            public void onClick(View v) {
                resetPosition();
            }
        });
        bar.addView(btnCenter);

        // Close button
        Button btnClose = new Button(this);
        btnClose.setText("✕");
        btnClose.setTextSize(10f);
        btnClose.setTextColor(Color.parseColor("#FF5252"));
        btnClose.setBackgroundColor(Color.parseColor("#25FF5252"));
        btnClose.setPadding(dp(6), dp(2), dp(6), dp(2));
        btnClose.setOnClickListener(new View.OnClickListener() {
            @Override
            public void onClick(View v) {
                stopSelf();
            }
        });
        bar.addView(btnClose);

        return bar;
    }

    public void setLockedState(boolean locked) {
        isLocked = locked;
        if (mParams != null && mWindowManager != null && mRootContainer != null) {
            int flags = WindowManager.LayoutParams.FLAG_NOT_FOCUSABLE
                    | WindowManager.LayoutParams.FLAG_LAYOUT_NO_LIMITS
                    | WindowManager.LayoutParams.FLAG_LAYOUT_IN_SCREEN
                    | WindowManager.LayoutParams.FLAG_HARDWARE_ACCELERATED;

            if (isLocked) {
                flags |= WindowManager.LayoutParams.FLAG_NOT_TOUCHABLE;
            } else {
                flags |= WindowManager.LayoutParams.FLAG_NOT_TOUCH_MODAL;
            }
            mParams.flags = flags;
            mParams.width = calculateWindowWidth();
            mParams.height = calculateWindowHeight();
            try {
                mWindowManager.updateViewLayout(mRootContainer, mParams);
            } catch (Exception e) {}

            if (mEditBar != null) {
                mEditBar.setVisibility(isLocked ? View.GONE : View.VISIBLE);
            }

            saveLockedState(isLocked);
            startForegroundNotification();

            String msg = isLocked
                    ? "AimHUD Locked: Touches pass through to game."
                    : "AimHUD Unlocked: Drag anywhere to reposition.";
            Toast.makeText(this, msg, Toast.LENGTH_SHORT).show();
        }
    }

    public void setMacroLockedState(boolean locked) {
        isMacroLocked = locked;
        if (mMacroParams != null && mWindowManager != null && mMacroContainer != null) {
            int flags = WindowManager.LayoutParams.FLAG_NOT_FOCUSABLE
                    | WindowManager.LayoutParams.FLAG_LAYOUT_NO_LIMITS
                    | WindowManager.LayoutParams.FLAG_LAYOUT_IN_SCREEN
                    | WindowManager.LayoutParams.FLAG_HARDWARE_ACCELERATED;

            if (isMacroLocked) {
                flags |= WindowManager.LayoutParams.FLAG_NOT_TOUCHABLE;
            } else {
                flags |= WindowManager.LayoutParams.FLAG_NOT_TOUCH_MODAL;
            }
            mMacroParams.flags = flags;
            try {
                mWindowManager.updateViewLayout(mMacroContainer, mMacroParams);
            } catch (Exception e) {}

            saveMacroLockedState(isMacroLocked);
            String msg = isMacroLocked
                    ? "Macro HUD Locked: Touches pass through to game."
                    : "Macro HUD Unlocked: Drag circular icon to reposition.";
            Toast.makeText(this, msg, Toast.LENGTH_SHORT).show();
        }
    }

    public void resetPosition() {
        mPosX = 0;
        mPosY = 0;
        if (mParams != null && mWindowManager != null && mRootContainer != null) {
            mParams.x = 0;
            mParams.y = 0;
            try {
                mWindowManager.updateViewLayout(mRootContainer, mParams);
            } catch (Exception e) {}
            savePosition(0, 0);
            Toast.makeText(this, "AimHUD centered at (0, 0)", Toast.LENGTH_SHORT).show();
        }
    }

    public void resetMacroPosition() {
        mMacroFireX = 140;
        mMacroFireY = 160;
        if (mMacroParams != null && mWindowManager != null && mMacroContainer != null) {
            mMacroParams.x = mMacroFireX;
            mMacroParams.y = mMacroFireY;
            try {
                mWindowManager.updateViewLayout(mMacroContainer, mMacroParams);
            } catch (Exception e) {}
            saveMacroPosition(mMacroFireX, mMacroFireY);
            Toast.makeText(this, "Macro HUD reset to position (140, 160)", Toast.LENGTH_SHORT).show();
        }
    }

    public void removeMacroOverlayDirect() {
        mShowMacroGuides = false;
        if (mMacroContainer != null && mWindowManager != null) {
            try {
                mWindowManager.removeView(mMacroContainer);
            } catch (Exception e) {}
            mMacroContainer = null;
        }
        saveFullConfig();
        Toast.makeText(this, "Macro HUD Overlay Removed (Settings saved)", Toast.LENGTH_SHORT).show();
        if (mRootContainer == null) {
            stopSelf();
        }
    }

    private void initMacroOverlayView() {
        if (mMacroContainer != null && mWindowManager != null) {
            try {
                mWindowManager.removeView(mMacroContainer);
            } catch (Exception e) {}
            mMacroContainer = null;
        }

        int layoutType;
        if (Build.VERSION.SDK_INT >= Build.VERSION_CODES.O) {
            layoutType = WindowManager.LayoutParams.TYPE_APPLICATION_OVERLAY;
        } else {
            layoutType = WindowManager.LayoutParams.TYPE_PHONE;
        }

        int flags = WindowManager.LayoutParams.FLAG_NOT_FOCUSABLE
                | WindowManager.LayoutParams.FLAG_LAYOUT_NO_LIMITS
                | WindowManager.LayoutParams.FLAG_LAYOUT_IN_SCREEN
                | WindowManager.LayoutParams.FLAG_HARDWARE_ACCELERATED;

        if (isMacroLocked) {
            flags |= WindowManager.LayoutParams.FLAG_NOT_TOUCHABLE;
        } else {
            flags |= WindowManager.LayoutParams.FLAG_NOT_TOUCH_MODAL;
        }

        int winSize = dp(mMacroFireSize * 2) + dp(28);
        mMacroParams = new WindowManager.LayoutParams(
                winSize,
                winSize,
                layoutType,
                flags,
                PixelFormat.TRANSLUCENT
        );

        if (Build.VERSION.SDK_INT >= Build.VERSION_CODES.P) {
            mMacroParams.layoutInDisplayCutoutMode = WindowManager.LayoutParams.LAYOUT_IN_DISPLAY_CUTOUT_MODE_SHORT_EDGES;
        }

        mMacroParams.gravity = Gravity.CENTER;
        mMacroParams.x = mMacroFireX;
        mMacroParams.y = mMacroFireY;

        mMacroContainer = new FrameLayout(this);
        mMacroContainer.setClipChildren(false);
        mMacroContainer.setClipToPadding(false);

        mDedicatedMacroView = new MacroLayoutView(this);
        mMacroContainer.addView(mDedicatedMacroView, new FrameLayout.LayoutParams(
                FrameLayout.LayoutParams.MATCH_PARENT,
                FrameLayout.LayoutParams.MATCH_PARENT,
                Gravity.CENTER
        ));

        // Touch listener on dedicated macro container for direct drag repositioning
        mMacroContainer.setOnTouchListener(new View.OnTouchListener() {
            private int initialX, initialY;
            private float initialTouchX, initialTouchY;
            private boolean isMoved = false;

            @Override
            public boolean onTouch(View v, MotionEvent event) {
                if (isMacroLocked) {
                    return false;
                }

                switch (event.getAction()) {
                    case MotionEvent.ACTION_DOWN:
                        initialX = mMacroParams.x;
                        initialY = mMacroParams.y;
                        initialTouchX = event.getRawX();
                        initialTouchY = event.getRawY();
                        isMoved = false;
                        return true;

                    case MotionEvent.ACTION_MOVE:
                        float dx = event.getRawX() - initialTouchX;
                        float dy = event.getRawY() - initialTouchY;
                        if (Math.abs(dx) > 3 || Math.abs(dy) > 3) {
                            isMoved = true;
                        }
                        mMacroParams.x = initialX + (int) dx;
                        mMacroParams.y = initialY + (int) dy;
                        mMacroFireX = mMacroParams.x;
                        mMacroFireY = mMacroParams.y;
                        try {
                            mWindowManager.updateViewLayout(mMacroContainer, mMacroParams);
                        } catch (Exception e) {}
                        return true;

                    case MotionEvent.ACTION_UP:
                        if (isMoved) {
                            saveMacroPosition(mMacroFireX, mMacroFireY);
                        }
                        return true;
                }
                return false;
            }
        });

        updateMacroOverlayAppearance();

        try {
            mWindowManager.addView(mMacroContainer, mMacroParams);
        } catch (Exception e) {
            mMacroContainer = null;
        }
    }

    private void updateMacroOverlayAppearance() {
        if (mDedicatedMacroView != null) {
            mDedicatedMacroView.update(true, mMacroFireSize, 0, 0, mMacroOpacity, mMacroColor);
        }
        if (mMacroParams != null && mWindowManager != null && mMacroContainer != null) {
            int winSize = dp(mMacroFireSize * 2) + dp(28);
            mMacroParams.width = winSize;
            mMacroParams.height = winSize;
            mMacroParams.x = mMacroFireX;
            mMacroParams.y = mMacroFireY;
            try {
                mWindowManager.updateViewLayout(mMacroContainer, mMacroParams);
            } catch (Exception e) {}
        }
    }

    private void updateOverlayAppearance() {
        if (mReticleView != null) {
            mReticleView.update(mStyle, mSizeDp, mOpacity, mRotation, mColor);
        }
        if (mMacroView != null) {
            mMacroView.update(mShowMacroGuides, mMacroFireSize, mMacroFireX, mMacroFireY, mMacroOpacity, mMacroColor);
        }
        if (mParams != null && mWindowManager != null && mRootContainer != null) {
            mParams.width = calculateWindowWidth();
            mParams.height = calculateWindowHeight();
            mParams.x = mPosX;
            mParams.y = mPosY;
            try {
                mWindowManager.updateViewLayout(mRootContainer, mParams);
            } catch (Exception e) {}
        }

        if (mShowMacroGuides) {
            if (mMacroContainer == null) {
                initMacroOverlayView();
            } else {
                updateMacroOverlayAppearance();
            }
        } else if (mMacroContainer != null && mWindowManager != null) {
            try {
                mWindowManager.removeView(mMacroContainer);
            } catch (Exception e) {}
            mMacroContainer = null;
        }
    }

    private void parseConfigJson(String jsonStr) {
        if (jsonStr == null || jsonStr.trim().isEmpty()) return;
        try {
            JSONObject obj = new JSONObject(jsonStr);
            if (obj.has("reticleEnabled")) mReticleEnabled = obj.getBoolean("reticleEnabled");
            if (obj.has("style")) mStyle = obj.getString("style");
            if (obj.has("size")) mSizeDp = obj.getInt("size");
            if (obj.has("opacity")) mOpacity = (float) (obj.getDouble("opacity") / 100.0);
            if (obj.has("rotation")) mRotation = (float) obj.getDouble("rotation");
            if (obj.has("color")) {
                String hex = obj.getString("color");
                if (hex.startsWith("#")) mColor = Color.parseColor(hex);
            }
            if (obj.has("posX")) mPosX = obj.getInt("posX");
            if (obj.has("posY")) mPosY = obj.getInt("posY");
            if (obj.has("locked")) isLocked = obj.getBoolean("locked");
            if (obj.has("showMacro")) mShowMacroGuides = obj.getBoolean("showMacro");
            if (obj.has("macroFireSize")) mMacroFireSize = obj.getInt("macroFireSize");
            if (obj.has("macroFireX")) mMacroFireX = obj.getInt("macroFireX");
            if (obj.has("macroFireY")) mMacroFireY = obj.getInt("macroFireY");
            if (obj.has("macroOpacity")) mMacroOpacity = (float) (obj.getDouble("macroOpacity") / 100.0);
            if (obj.has("macroColor")) {
                String mhex = obj.getString("macroColor");
                if (mhex.startsWith("#")) mMacroColor = Color.parseColor(mhex);
            }
            if (obj.has("macroLocked")) isMacroLocked = obj.getBoolean("macroLocked");

            saveFullConfig();
        } catch (Exception e) {}
    }

    private void savePosition(int x, int y) {
        SharedPreferences prefs = getSharedPreferences("sensi_aimhud_prefs", Context.MODE_PRIVATE);
        prefs.edit().putInt("pos_x", x).putInt("pos_y", y).apply();
    }

    private void saveMacroPosition(int x, int y) {
        SharedPreferences prefs = getSharedPreferences("sensi_aimhud_prefs", Context.MODE_PRIVATE);
        prefs.edit().putInt("macro_fire_x", x).putInt("macro_fire_y", y).apply();
    }

    private void saveLockedState(boolean locked) {
        SharedPreferences prefs = getSharedPreferences("sensi_aimhud_prefs", Context.MODE_PRIVATE);
        prefs.edit().putBoolean("locked", locked).apply();
    }

    private void saveMacroLockedState(boolean locked) {
        SharedPreferences prefs = getSharedPreferences("sensi_aimhud_prefs", Context.MODE_PRIVATE);
        prefs.edit().putBoolean("macro_locked", locked).apply();
    }

    private void saveFullConfig() {
        SharedPreferences prefs = getSharedPreferences("sensi_aimhud_prefs", Context.MODE_PRIVATE);
        SharedPreferences.Editor ed = prefs.edit();
        ed.putBoolean("reticle_enabled", mReticleEnabled);
        ed.putString("style", mStyle);
        ed.putInt("size", mSizeDp);
        ed.putFloat("opacity", mOpacity);
        ed.putFloat("rotation", mRotation);
        ed.putInt("color", mColor);
        ed.putInt("pos_x", mPosX);
        ed.putInt("pos_y", mPosY);
        ed.putBoolean("locked", isLocked);
        ed.putBoolean("show_macro", mShowMacroGuides);
        ed.putInt("macro_fire_size", mMacroFireSize);
        ed.putFloat("macro_opacity", mMacroOpacity);
        ed.putInt("macro_color", mMacroColor);
        ed.putInt("macro_fire_x", mMacroFireX);
        ed.putInt("macro_fire_y", mMacroFireY);
        ed.putBoolean("macro_locked", isMacroLocked);
        ed.apply();
    }

    private void loadSavedConfig() {
        SharedPreferences prefs = getSharedPreferences("sensi_aimhud_prefs", Context.MODE_PRIVATE);
        mReticleEnabled = prefs.getBoolean("reticle_enabled", true);
        mStyle = prefs.getString("style", "plus");
        mSizeDp = prefs.getInt("size", 36);
        mOpacity = prefs.getFloat("opacity", 0.85f);
        mRotation = prefs.getFloat("rotation", 0f);
        mColor = prefs.getInt("color", Color.parseColor("#00E5FF"));
        mPosX = prefs.getInt("pos_x", 0);
        mPosY = prefs.getInt("pos_y", 0);
        isLocked = prefs.getBoolean("locked", true);
        mShowMacroGuides = prefs.getBoolean("show_macro", false);
        mMacroFireSize = prefs.getInt("macro_fire_size", 48);
        mMacroOpacity = prefs.getFloat("macro_opacity", 0.85f);
        mMacroColor = prefs.getInt("macro_color", Color.parseColor("#00E676"));
        mMacroFireX = prefs.getInt("macro_fire_x", 140);
        mMacroFireY = prefs.getInt("macro_fire_y", 160);
        isMacroLocked = prefs.getBoolean("macro_locked", true);
    }

    private int dp(int pixels) {
        return (int) (pixels * getResources().getDisplayMetrics().density + 0.5f);
    }

    private int dp(float pixels) {
        return (int) (pixels * getResources().getDisplayMetrics().density + 0.5f);
    }

    @Override
    public void onDestroy() {
        super.onDestroy();
        isRunning = false;
        if (mRootContainer != null && mWindowManager != null) {
            try {
                mWindowManager.removeView(mRootContainer);
            } catch (Exception e) {}
            mRootContainer = null;
        }
        if (mMacroContainer != null && mWindowManager != null) {
            try {
                mWindowManager.removeView(mMacroContainer);
            } catch (Exception e) {}
            mMacroContainer = null;
        }
        stopForeground(true);
    }

    @Override
    public void onConfigurationChanged(android.content.res.Configuration newConfig) {
        super.onConfigurationChanged(newConfig);
        if (mRootContainer != null && mWindowManager != null && mParams != null) {
            try {
                mWindowManager.updateViewLayout(mRootContainer, mParams);
            } catch (Exception e) {}
        }
        if (mMacroContainer != null && mWindowManager != null && mMacroParams != null) {
            try {
                mWindowManager.updateViewLayout(mMacroContainer, mMacroParams);
            } catch (Exception e) {}
        }
    }

    // =========================================================================
    // CUSTOM VIEW: AIM RETICLE (CANVAS RENDERING)
    // =========================================================================
    public class AimReticleView extends View {
        private String style = "plus";
        private int sizeDp = 36;
        private float opacity = 0.85f;
        private float rotation = 0f;
        private int color = Color.parseColor("#00E5FF");
        private Paint paint;
        private Paint fillPaint;

        public AimReticleView(Context context) {
            super(context);
            init();
        }

        private void init() {
            paint = new Paint(Paint.ANTI_ALIAS_FLAG);
            paint.setStyle(Paint.Style.STROKE);
            paint.setStrokeCap(Paint.Cap.ROUND);

            fillPaint = new Paint(Paint.ANTI_ALIAS_FLAG);
            fillPaint.setStyle(Paint.Style.FILL);
        }

        public void update(String style, int sizeDp, float opacity, float rotation, int color) {
            this.style = style;
            this.sizeDp = Math.max(12, Math.min(120, sizeDp));
            this.opacity = Math.max(0.2f, Math.min(1.0f, opacity));
            this.rotation = rotation;
            this.color = color;

            int pxSize = dp(this.sizeDp * 2);
            setLayoutParams(new FrameLayout.LayoutParams(pxSize, pxSize, Gravity.CENTER));
            invalidate();
        }

        @Override
        protected void onMeasure(int widthMeasureSpec, int heightMeasureSpec) {
            int desired = dp(sizeDp * 2);
            int width = resolveSize(desired, widthMeasureSpec);
            int height = resolveSize(desired, heightMeasureSpec);
            setMeasuredDimension(width, height);
        }

        @Override
        protected void onDraw(Canvas canvas) {
            super.onDraw(canvas);

            int w = getWidth();
            int h = getHeight();
            if (w == 0 || h == 0) return;

            float cx = w / 2f;
            float cy = h / 2f;
            float r = dp(sizeDp) * 0.45f;
            int alpha = (int) (opacity * 255);

            paint.setColor(color);
            paint.setAlpha(alpha);
            float strokeW = Math.max(dp(2), dp(sizeDp) * 0.07f);
            paint.setStrokeWidth(strokeW);

            fillPaint.setColor(color);
            fillPaint.setAlpha(alpha);

            canvas.save();
            canvas.rotate(rotation, cx, cy);

            switch (style.toLowerCase()) {
                case "circle":
                    // Reticle Circle: Outer circle with center precision dot & 4 tick marks
                    canvas.drawCircle(cx, cy, r, paint);
                    canvas.drawCircle(cx, cy, strokeW * 1.2f, fillPaint);
                    // 4 tick lines
                    canvas.drawLine(cx, cy - r - dp(4), cx, cy - r + dp(3), paint);
                    canvas.drawLine(cx, cy + r - dp(3), cx, cy + r + dp(4), paint);
                    canvas.drawLine(cx - r - dp(4), cy, cx - r + dp(3), cy, paint);
                    canvas.drawLine(cx + r - dp(3), cy, cx + r + dp(4), cy, paint);
                    break;

                case "dot":
                    // Precision Dot: Center solid dot with subtle faint halo
                    canvas.drawCircle(cx, cy, strokeW * 2.2f, fillPaint);
                    paint.setAlpha((int) (alpha * 0.4f));
                    canvas.drawCircle(cx, cy, strokeW * 4.2f, paint);
                    break;

                case "cross":
                    // Diagonal 'X' Crosshair
                    float offsetInner = r * 0.25f;
                    float offsetOuter = r * 0.95f;
                    canvas.drawLine(cx - offsetOuter, cy - offsetOuter, cx - offsetInner, cy - offsetInner, paint);
                    canvas.drawLine(cx + offsetInner, cy + offsetInner, cx + offsetOuter, cy + offsetOuter, paint);
                    canvas.drawLine(cx + offsetOuter, cy - offsetOuter, cx + offsetInner, cy - offsetInner, paint);
                    canvas.drawLine(cx - offsetInner, cy + offsetInner, cx - offsetOuter, cy + offsetOuter, paint);
                    canvas.drawCircle(cx, cy, strokeW * 0.9f, fillPaint);
                    break;

                case "ring":
                    // Open Ring: Thin precision ring with center pip
                    paint.setStrokeWidth(Math.max((float)dp(2), dp(sizeDp) * 0.05f));
                    canvas.drawCircle(cx, cy, r * 0.85f, paint);
                    canvas.drawCircle(cx, cy, strokeW * 0.8f, fillPaint);
                    break;

                case "plus":
                default:
                    // Classic '+' Crosshair with center dot and 4 arms
                    float innerGap = r * 0.3f;
                    float outerArm = r * 1.05f;
                    canvas.drawLine(cx, cy - outerArm, cx, cy - innerGap, paint);
                    canvas.drawLine(cx, cy + innerGap, cx, cy + outerArm, paint);
                    canvas.drawLine(cx - outerArm, cy, cx - innerGap, cy, paint);
                    canvas.drawLine(cx + innerGap, cy, cx + outerArm, cy, paint);
                    canvas.drawCircle(cx, cy, strokeW * 0.9f, fillPaint);
                    break;
            }

            canvas.restore();
        }
    }

    // =========================================================================
    // CUSTOM VIEW: CIRCULAR MACRO HUD ICON & CALIBRATION GUIDES
    // =========================================================================
    public class MacroLayoutView extends View {
        private boolean showGuides = false;
        private int fireSize = 48;
        private int fireX = 140;
        private int fireY = 160;
        private float opacity = 0.85f;
        private int color = Color.parseColor("#00E676");
        private Paint ringPaint;
        private Paint textPaint;
        private Paint subTextPaint;

        public MacroLayoutView(Context context) {
            super(context);
            ringPaint = new Paint(Paint.ANTI_ALIAS_FLAG);
            ringPaint.setStyle(Paint.Style.STROKE);
            ringPaint.setColor(color);

            textPaint = new Paint(Paint.ANTI_ALIAS_FLAG);
            textPaint.setColor(color);
            textPaint.setTextAlign(Paint.Align.CENTER);
            textPaint.setTextSize(dp(12));
            textPaint.setFakeBoldText(true);

            subTextPaint = new Paint(Paint.ANTI_ALIAS_FLAG);
            subTextPaint.setColor(color);
            subTextPaint.setTextAlign(Paint.Align.CENTER);
            subTextPaint.setTextSize(dp(9));
            subTextPaint.setFakeBoldText(true);
        }

        public void update(boolean show, int fireSize, int fireX, int fireY, float opacity) {
            update(show, fireSize, fireX, fireY, opacity, this.color);
        }

        public void update(boolean show, int fireSize, int fireX, int fireY, float opacity, int color) {
            this.showGuides = show;
            this.fireSize = Math.max(24, Math.min(120, fireSize));
            this.fireX = fireX;
            this.fireY = fireY;
            this.opacity = Math.max(0.2f, Math.min(1.0f, opacity));
            this.color = color;
            setVisibility(show ? View.VISIBLE : View.GONE);
            if (show) {
                int pxSize = dp(this.fireSize * 2) + dp(24);
                setLayoutParams(new FrameLayout.LayoutParams(pxSize, pxSize, Gravity.CENTER));
                invalidate();
            }
        }

        @Override
        protected void onMeasure(int widthMeasureSpec, int heightMeasureSpec) {
            if (!showGuides) {
                setMeasuredDimension(0, 0);
                return;
            }
            int desired = dp(fireSize * 2) + dp(24);
            int width = resolveSize(desired, widthMeasureSpec);
            int height = resolveSize(desired, heightMeasureSpec);
            setMeasuredDimension(width, height);
        }

        @Override
        protected void onDraw(Canvas canvas) {
            super.onDraw(canvas);
            if (!showGuides) return;

            int w = getWidth();
            int h = getHeight();
            if (w == 0 || h == 0) return;

            float cx = w / 2f;
            float cy = h / 2f;
            float r = dp(fireSize) * 0.78f;
            int alpha = (int) (opacity * 255);

            // Outer primary circular ring
            ringPaint.setColor(color);
            ringPaint.setAlpha(alpha);
            ringPaint.setStrokeWidth(dp(2.5f));
            canvas.drawCircle(cx, cy, r, ringPaint);

            // Secondary concentric guide ring
            ringPaint.setStrokeWidth(dp(1.2f));
            ringPaint.setAlpha((int) (alpha * 0.5f));
            canvas.drawCircle(cx, cy, r * 0.65f, ringPaint);

            // 4 alignment tick marks
            ringPaint.setStrokeWidth(dp(2f));
            ringPaint.setAlpha(alpha);
            canvas.drawLine(cx, cy - r, cx, cy - r + dp(6), ringPaint);
            canvas.drawLine(cx, cy + r - dp(6), cx, cy + r, ringPaint);
            canvas.drawLine(cx - r, cy, cx - r + dp(6), cy, ringPaint);
            canvas.drawLine(cx + r - dp(6), cy, cx + r, cy, ringPaint);

            // Center 'M' Insignia
            textPaint.setColor(color);
            textPaint.setAlpha(alpha);
            textPaint.setTextSize(Math.max(dp(11), dp(fireSize) * 0.32f));
            canvas.drawText("M", cx, cy + dp(4), textPaint);

            // Calibration HUD bottom label
            subTextPaint.setColor(color);
            subTextPaint.setAlpha((int) (alpha * 0.85f));
            canvas.drawText("FIRE BUTTON (" + fireSize + "%)", cx, cy + r + dp(10), subTextPaint);
        }
    }
}
