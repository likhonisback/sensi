package com.sensipro.app;

import android.app.Service;
import android.content.Context;
import android.content.Intent;
import android.content.SharedPreferences;
import android.content.pm.PackageManager;
import android.widget.Toast;
import android.graphics.Color;
import android.graphics.PixelFormat;
import android.graphics.Typeface;
import android.graphics.drawable.GradientDrawable;
import android.os.Build;
import android.os.IBinder;
import android.provider.Settings;
import android.view.Gravity;
import android.view.MotionEvent;
import android.view.View;
import android.view.WindowManager;
import android.widget.Button;
import android.widget.FrameLayout;
import android.widget.GridLayout;
import android.widget.LinearLayout;
import android.widget.TextView;
import org.json.JSONObject;

public class FloatingGuideService extends Service {

    public static final String ACTION_START = "com.sensipro.app.action.START_FLOATING_GUIDE";
    public static final String ACTION_STOP = "com.sensipro.app.action.STOP_FLOATING_GUIDE";
    public static final String ACTION_UPDATE = "com.sensipro.app.action.UPDATE_FLOATING_GUIDE";
    public static boolean isRunning = false;

    private WindowManager mWindowManager;
    private View mFloatingView;
    private WindowManager.LayoutParams mParams;
    private TextView mTitleText;
    private TextView mGeneralVal, mRedDotVal, mScope2xVal, mScope4xVal, mSniperVal, mFreeLookVal, mButtonVal, mDpiVal;
    private View mContentView;
    private Button mToggleBtn;
    private boolean mIsCollapsed = false;

    @Override
    public IBinder onBind(Intent intent) {
        return null;
    }

    @Override
    public int onStartCommand(Intent intent, int flags, int startId) {
        if (intent != null) {
            String action = intent.getAction();
            if (ACTION_STOP.equals(action)) {
                stopSelf();
                return START_NOT_STICKY;
            } else if (ACTION_UPDATE.equals(action)) {
                refreshSavedValues();
                return START_STICKY;
            }
        }

        // Verify overlay permission before continuing
        if (Build.VERSION.SDK_INT >= Build.VERSION_CODES.M && !Settings.canDrawOverlays(this)) {
            stopSelf();
            return START_NOT_STICKY;
        }

        if (mFloatingView == null) {
            createFloatingView();
        }
        refreshSavedValues();
        isRunning = true;
        return START_STICKY;
    }

    private void createFloatingView() {
        mWindowManager = (WindowManager) getSystemService(WINDOW_SERVICE);

        int layoutType;
        if (Build.VERSION.SDK_INT >= Build.VERSION_CODES.O) {
            layoutType = WindowManager.LayoutParams.TYPE_APPLICATION_OVERLAY;
        } else {
            layoutType = WindowManager.LayoutParams.TYPE_PHONE;
        }

        mParams = new WindowManager.LayoutParams(
                WindowManager.LayoutParams.WRAP_CONTENT,
                WindowManager.LayoutParams.WRAP_CONTENT,
                layoutType,
                WindowManager.LayoutParams.FLAG_NOT_FOCUSABLE | WindowManager.LayoutParams.FLAG_NOT_TOUCH_MODAL,
                PixelFormat.TRANSLUCENT
        );

        mParams.gravity = Gravity.TOP | Gravity.START;
        mParams.x = 40;
        mParams.y = 150;

        // Create main container
        LinearLayout container = new LinearLayout(this);
        container.setOrientation(LinearLayout.VERTICAL);
        container.setPadding(dp(10), dp(8), dp(10), dp(8));

        // Dark modern background with neon cyan border
        GradientDrawable bg = new GradientDrawable();
        bg.setColor(Color.parseColor("#EC0E121E"));
        bg.setCornerRadius(dp(12));
        bg.setStroke(dp(1), Color.parseColor("#3800E5FF"));
        container.setBackground(bg);

        // Header (Title, Launch Game, Toggle Show/Hide, and Close button)
        LinearLayout header = new LinearLayout(this);
        header.setOrientation(LinearLayout.HORIZONTAL);
        header.setGravity(Gravity.CENTER_VERTICAL);
        header.setLayoutParams(new LinearLayout.LayoutParams(
                LinearLayout.LayoutParams.MATCH_PARENT,
                LinearLayout.LayoutParams.WRAP_CONTENT
        ));

        mTitleText = new TextView(this);
        mTitleText.setText("⚡ SENSI GUIDE");
        mTitleText.setTextColor(Color.parseColor("#00E5FF"));
        mTitleText.setTextSize(11);
        mTitleText.setTypeface(Typeface.DEFAULT_BOLD);
        LinearLayout.LayoutParams titleParams = new LinearLayout.LayoutParams(0, LinearLayout.LayoutParams.WRAP_CONTENT, 1.0f);
        mTitleText.setLayoutParams(titleParams);
        header.addView(mTitleText);

        // One-tap Free Fire Launch Button on Floating Overlay
        Button launchBtn = new Button(this);
        launchBtn.setText("🎮");
        launchBtn.setTextSize(11);
        launchBtn.setTextColor(Color.parseColor("#00E676"));
        launchBtn.setBackgroundColor(Color.TRANSPARENT);
        launchBtn.setPadding(dp(2), 0, dp(2), 0);
        LinearLayout.LayoutParams launchParams = new LinearLayout.LayoutParams(dp(28), dp(28));
        launchParams.setMargins(0, 0, dp(2), 0);
        launchBtn.setLayoutParams(launchParams);
        launchBtn.setOnClickListener(new View.OnClickListener() {
            @Override
            public void onClick(View v) {
                launchTargetGame();
            }
        });
        header.addView(launchBtn);

        // Minimize / Collapse Button
        mToggleBtn = new Button(this);
        mToggleBtn.setText("–");
        mToggleBtn.setTextSize(12);
        mToggleBtn.setTextColor(Color.parseColor("#C4CCDF"));
        mToggleBtn.setBackgroundColor(Color.TRANSPARENT);
        mToggleBtn.setPadding(dp(4), 0, dp(4), 0);
        LinearLayout.LayoutParams btnParams = new LinearLayout.LayoutParams(dp(28), dp(28));
        btnParams.setMargins(0, 0, dp(4), 0);
        mToggleBtn.setLayoutParams(btnParams);
        mToggleBtn.setOnClickListener(new View.OnClickListener() {
            @Override
            public void onClick(View v) {
                toggleCollapse();
            }
        });
        header.addView(mToggleBtn);

        // Close Button
        Button closeBtn = new Button(this);
        closeBtn.setText("✕");
        closeBtn.setTextSize(11);
        closeBtn.setTextColor(Color.parseColor("#FF5252"));
        closeBtn.setBackgroundColor(Color.TRANSPARENT);
        closeBtn.setPadding(dp(4), 0, dp(4), 0);
        LinearLayout.LayoutParams closeParams = new LinearLayout.LayoutParams(dp(28), dp(28));
        closeBtn.setLayoutParams(closeParams);
        closeBtn.setOnClickListener(new View.OnClickListener() {
            @Override
            public void onClick(View v) {
                stopSelf();
            }
        });
        header.addView(closeBtn);

        container.addView(header);

        // Content Area (Grid of values)
        LinearLayout content = new LinearLayout(this);
        content.setOrientation(LinearLayout.VERTICAL);
        content.setLayoutParams(new LinearLayout.LayoutParams(
                LinearLayout.LayoutParams.MATCH_PARENT,
                LinearLayout.LayoutParams.WRAP_CONTENT
        ));
        content.setPadding(0, dp(6), 0, 0);
        mContentView = content;

        // Row 1: General & Red Dot
        LinearLayout row1 = new LinearLayout(this);
        row1.setOrientation(LinearLayout.HORIZONTAL);
        mGeneralVal = addMetricBadge(row1, "General:", "--", "#FF2B56");
        mRedDotVal = addMetricBadge(row1, "Red Dot:", "--", "#FF2B56");
        content.addView(row1);

        // Row 2: 2x & 4x Scope
        LinearLayout row2 = new LinearLayout(this);
        row2.setOrientation(LinearLayout.HORIZONTAL);
        row2.setPadding(0, dp(4), 0, 0);
        mScope2xVal = addMetricBadge(row2, "2x Scope:", "--", "#00E5FF");
        mScope4xVal = addMetricBadge(row2, "4x Scope:", "--", "#00E5FF");
        content.addView(row2);

        // Row 3: Sniper Scope & Free Look
        LinearLayout row3 = new LinearLayout(this);
        row3.setOrientation(LinearLayout.HORIZONTAL);
        row3.setPadding(0, dp(4), 0, 0);
        mSniperVal = addMetricBadge(row3, "Sniper:", "--", "#FFD600");
        mFreeLookVal = addMetricBadge(row3, "Free Look:", "--", "#FF7043");
        content.addView(row3);

        // Row 4: Fire Button Size & Target DPI
        LinearLayout row4 = new LinearLayout(this);
        row4.setOrientation(LinearLayout.HORIZONTAL);
        row4.setPadding(0, dp(4), 0, 0);
        mButtonVal = addMetricBadge(row4, "Button:", "--", "#00E676");
        mDpiVal = addMetricBadge(row4, "DPI:", "--", "#B388FF");
        content.addView(row4);

        container.addView(content);

        // Make floating container draggable
        container.setOnTouchListener(new View.OnTouchListener() {
            private int initialX;
            private int initialY;
            private float initialTouchX;
            private float initialTouchY;
            private boolean isClick = true;

            @Override
            public boolean onTouch(View v, MotionEvent event) {
                switch (event.getAction()) {
                    case MotionEvent.ACTION_DOWN:
                        initialX = mParams.x;
                        initialY = mParams.y;
                        initialTouchX = event.getRawX();
                        initialTouchY = event.getRawY();
                        isClick = true;
                        return true;

                    case MotionEvent.ACTION_MOVE:
                        float dx = event.getRawX() - initialTouchX;
                        float dy = event.getRawY() - initialTouchY;
                        if (Math.abs(dx) > 10 || Math.abs(dy) > 10) {
                            isClick = false;
                        }
                        mParams.x = initialX + (int) dx;
                        mParams.y = initialY + (int) dy;
                        try {
                            mWindowManager.updateViewLayout(mFloatingView, mParams);
                        } catch (Exception e) {}
                        return true;

                    case MotionEvent.ACTION_UP:
                        if (isClick && mIsCollapsed) {
                            toggleCollapse();
                        }
                        return true;
                }
                return false;
            }
        });

        mFloatingView = container;
        try {
            mWindowManager.addView(mFloatingView, mParams);
        } catch (Exception e) {
            mFloatingView = null;
            isRunning = false;
            stopSelf();
        }
    }

    private TextView addMetricBadge(LinearLayout row, String label, String initialVal, String hexColor) {
        LinearLayout box = new LinearLayout(this);
        box.setOrientation(LinearLayout.HORIZONTAL);
        box.setGravity(Gravity.CENTER_VERTICAL);
        box.setPadding(dp(4), dp(2), dp(4), dp(2));
        LinearLayout.LayoutParams lp = new LinearLayout.LayoutParams(0, LinearLayout.LayoutParams.WRAP_CONTENT, 1.0f);
        box.setLayoutParams(lp);

        TextView lbl = new TextView(this);
        lbl.setText(label + " ");
        lbl.setTextSize(10);
        lbl.setTextColor(Color.parseColor("#8E9BB5"));
        box.addView(lbl);

        TextView val = new TextView(this);
        val.setText(initialVal);
        val.setTextSize(10.5f);
        val.setTypeface(Typeface.DEFAULT_BOLD);
        val.setTextColor(Color.parseColor(hexColor));
        box.addView(val);

        row.addView(box);
        return val;
    }

    private void toggleCollapse() {
        mIsCollapsed = !mIsCollapsed;
        if (mContentView != null) {
            mContentView.setVisibility(mIsCollapsed ? View.GONE : View.VISIBLE);
        }
        if (mToggleBtn != null) {
            mToggleBtn.setText(mIsCollapsed ? "+" : "–");
        }
        if (mTitleText != null) {
            mTitleText.setText(mIsCollapsed ? "⚡ SENSI" : "⚡ SENSI GUIDE");
        }
    }

    private void refreshSavedValues() {
        try {
            SharedPreferences prefs = getSharedPreferences("sensi_permanent_config", Context.MODE_PRIVATE);
            String globalPrefs = prefs.getString("sensee_global_prefs", null);
            String targetDevice = null;
            if (globalPrefs != null) {
                JSONObject gObj = new JSONObject(globalPrefs);
                if (gObj.has("deviceModel")) {
                    targetDevice = gObj.getString("deviceModel");
                }
            }

            String allConfigsJson = prefs.getString("sensi_device_configs", null);
            if (allConfigsJson != null) {
                JSONObject configsObj = new JSONObject(allConfigsJson);
                JSONObject devConfig = null;

                if (targetDevice != null && configsObj.has(targetDevice)) {
                    devConfig = configsObj.getJSONObject(targetDevice);
                } else if (configsObj.length() > 0) {
                    // Fallback to first available device
                    String firstKey = configsObj.keys().next();
                    devConfig = configsObj.getJSONObject(firstKey);
                    targetDevice = firstKey;
                }

                if (devConfig != null && devConfig.has("sensi")) {
                    JSONObject s = devConfig.getJSONObject("sensi");
                    final String devName = targetDevice != null ? targetDevice : "Device";
                    final String gen = String.valueOf(s.optInt("General", 175));
                    final String red = String.valueOf(s.optInt("RedDot", 165));
                    final String s2 = String.valueOf(s.optInt("Scope2x", 158));
                    final String s4 = String.valueOf(s.optInt("Scope4x", 162));
                    final String snip = String.valueOf(s.optInt("Sniper", 72));
                    final String freelook = String.valueOf(s.optInt("FreeLook", 135));
                    final String btn = s.optInt("ButtonSize", 48) + "%";
                    int dpiVal = s.optInt("Dpi", 0);
                    if (dpiVal <= 0) {
                        dpiVal = devConfig.optInt("Dpi", 0);
                    }
                    final String dpiStr = (dpiVal > 0) ? (dpiVal + " DPI") : "Default";

                    if (mGeneralVal != null) mGeneralVal.setText(gen);
                    if (mRedDotVal != null) mRedDotVal.setText(red);
                    if (mScope2xVal != null) mScope2xVal.setText(s2);
                    if (mScope4xVal != null) mScope4xVal.setText(s4);
                    if (mSniperVal != null) mSniperVal.setText(snip);
                    if (mFreeLookVal != null) mFreeLookVal.setText(freelook);
                    if (mButtonVal != null) mButtonVal.setText(btn);
                    if (mDpiVal != null) mDpiVal.setText(dpiStr);
                    if (mTitleText != null && !mIsCollapsed) {
                        mTitleText.setText("⚡ " + devName);
                    }
                    return;
                }
            }
        } catch (Exception e) {}

        // Fallback default message if no saved config exists
        if (mGeneralVal != null) mGeneralVal.setText("--");
        if (mRedDotVal != null) mRedDotVal.setText("--");
        if (mScope2xVal != null) mScope2xVal.setText("--");
        if (mScope4xVal != null) mScope4xVal.setText("--");
        if (mSniperVal != null) mSniperVal.setText("--");
        if (mFreeLookVal != null) mFreeLookVal.setText("--");
        if (mButtonVal != null) mButtonVal.setText("--");
        if (mDpiVal != null) mDpiVal.setText("--");
        if (mTitleText != null && !mIsCollapsed) mTitleText.setText("⚡ NO SAVED SENS");
    }

    private void launchTargetGame() {
        try {
            PackageManager pm = getPackageManager();
            String[] targetPackages = {"com.dts.freefiremax", "com.dts.freefireth"};
            for (String pkg : targetPackages) {
                Intent intent = pm.getLaunchIntentForPackage(pkg);
                if (intent != null) {
                    intent.addFlags(Intent.FLAG_ACTIVITY_NEW_TASK);
                    startActivity(intent);
                    return;
                }
            }
            Toast.makeText(this, "Free Fire / Free Fire MAX not installed on this device.", Toast.LENGTH_SHORT).show();
        } catch (Exception e) {}
    }

    private int dp(int pixels) {
        return (int) (pixels * getResources().getDisplayMetrics().density + 0.5f);
    }

    @Override
    public void onDestroy() {
        super.onDestroy();
        isRunning = false;
        if (mFloatingView != null && mWindowManager != null) {
            try {
                mWindowManager.removeView(mFloatingView);
            } catch (Exception e) {}
            mFloatingView = null;
        }
    }
}
