package com.sensipro.app;

import android.app.Activity;
import android.app.ActivityManager;
import android.content.Context;
import android.content.Intent;
import android.content.IntentFilter;
import android.content.pm.PackageManager;
import android.os.BatteryManager;
import android.os.Build;
import android.os.Bundle;
import android.os.Vibrator;
import android.webkit.JavascriptInterface;
import android.webkit.WebChromeClient;
import android.webkit.WebSettings;
import android.webkit.WebView;
import android.webkit.WebViewClient;
import android.provider.Settings;
import android.content.SharedPreferences;
import java.io.BufferedReader;
import java.io.InputStream;
import java.io.InputStreamReader;
import java.io.OutputStream;
import java.net.HttpURLConnection;
import java.net.URL;
import java.util.UUID;
import android.widget.Toast;
import android.view.View;
import android.view.WindowManager;
import android.view.Display;
import android.util.DisplayMetrics;
import android.graphics.Color;

public class MainActivity extends Activity {

    private WebView mWebView;

    @Override
    protected void onCreate(Bundle savedInstanceState) {
        super.onCreate(savedInstanceState);

        // Hardware-accelerated window for smooth rendering and high touch framerates
        getWindow().setFlags(
            WindowManager.LayoutParams.FLAG_HARDWARE_ACCELERATED,
            WindowManager.LayoutParams.FLAG_HARDWARE_ACCELERATED
        );

        mWebView = new WebView(this);
        mWebView.setLayerType(View.LAYER_TYPE_HARDWARE, null);
        mWebView.setVerticalScrollBarEnabled(false);
        mWebView.setHorizontalScrollBarEnabled(false);
        mWebView.setOverScrollMode(View.OVER_SCROLL_NEVER);
        setContentView(mWebView);

        WebSettings settings = mWebView.getSettings();
        settings.setJavaScriptEnabled(true);
        settings.setDomStorageEnabled(true);
        settings.setAllowFileAccess(true);
        settings.setAllowContentAccess(true);
        settings.setDatabaseEnabled(true);
        settings.setUseWideViewPort(true);
        settings.setLoadWithOverviewMode(true);
        settings.setCacheMode(WebSettings.LOAD_DEFAULT);

        mWebView.setWebViewClient(new WebViewClient() {
            @Override
            public boolean shouldOverrideUrlLoading(WebView view, String url) {
                if (url != null && (url.startsWith("http://") || url.startsWith("https://"))) {
                    try {
                        Intent intent = new Intent(Intent.ACTION_VIEW, android.net.Uri.parse(url));
                        startActivity(intent);
                        return true;
                    } catch (Exception e) {}
                }
                return false;
            }
        });
        mWebView.setWebChromeClient(new WebChromeClient());
        mWebView.addJavascriptInterface(new WebAppInterface(this), "AndroidBridge");

        mWebView.loadUrl("file:///android_asset/index.html");
    }

    public class WebAppInterface {
        Context mContext;

        WebAppInterface(Context c) {
            mContext = c;
        }

        @JavascriptInterface
        public String getDeviceModel() {
            String manufacturer = Build.MANUFACTURER != null ? Build.MANUFACTURER : "";
            String model = Build.MODEL != null ? Build.MODEL : "";

            // Mapping known internal hardware codes to clean commercial names
            // Samsung
            if (model.startsWith("SM-A155") || model.startsWith("SM-A156")) return "Samsung Galaxy A15";
            if (model.startsWith("SM-A346")) return "Samsung Galaxy A34 5G";
            if (model.startsWith("SM-A546")) return "Samsung Galaxy A54 5G";
            if (model.startsWith("SM-A736")) return "Samsung Galaxy A73 5G";
            if (model.startsWith("SM-G998")) return "Samsung Galaxy S21 Ultra";
            if (model.startsWith("SM-S908")) return "Samsung Galaxy S22 Ultra";
            if (model.startsWith("SM-S911")) return "Samsung Galaxy S23";
            if (model.startsWith("SM-S918")) return "Samsung Galaxy S23 Ultra";
            if (model.startsWith("SM-S921")) return "Samsung Galaxy S24";
            if (model.startsWith("SM-S928")) return "Samsung Galaxy S24 Ultra";
            if (model.startsWith("SM-F946")) return "Samsung Galaxy Z Fold 5";
            if (model.startsWith("SM-F731")) return "Samsung Galaxy Z Flip 5";

            // Xiaomi / Redmi / Poco
            if (model.contains("M2102J20SG") || model.contains("Poco X3 Pro")) return "Xiaomi Poco X3 Pro";
            if (model.contains("22041216G") || model.contains("Poco X4 GT")) return "Xiaomi Poco X4 GT";
            if (model.contains("23049PCD8G") || model.contains("Poco F5")) return "Xiaomi Poco F5";
            if (model.contains("23013PC75G") || model.contains("Poco F5 Pro")) return "Xiaomi Poco F5 Pro";
            if (model.contains("2201116SG") || model.contains("Redmi Note 11 Pro")) return "Redmi Note 11 Pro";
            if (model.contains("22101316UG") || model.contains("Redmi Note 12 Pro+")) return "Redmi Note 12 Pro+";
            if (model.startsWith("23124RA7")) return "Redmi Note 13";
            if (model.equals("2312DRA50G") || model.equals("2312DRA50I") || model.startsWith("2312DRA50")) return "Redmi Note 13 Pro";
            if (model.startsWith("2210132G")) return "Xiaomi 13 Pro";
            if (model.startsWith("23127PN0CG")) return "Xiaomi 14";

            // Realme
            if (model.equalsIgnoreCase("RMX3241")) return "Realme 8 5G (RMX3241)";
            if (model.equalsIgnoreCase("RMX3085") || model.equalsIgnoreCase("RMX3081")) return "Realme 8";
            if (model.contains("RMX3392") || model.contains("Realme 9 Pro+")) return "Realme 9 Pro+";
            if (model.contains("RMX3740") || model.contains("Realme 11 Pro+")) return "Realme 11 Pro+";
            if (model.contains("RMX3560") || model.contains("GT Neo 3")) return "Realme GT Neo 3";
            if (model.contains("RMX3286") || model.contains("Narzo 50")) return "Realme Narzo 50";
            if (model.contains("RMX2202") || model.contains("Realme GT")) return "Realme GT 5G";

            // Vivo / iQOO
            if (model.contains("V2170A") || model.contains("iQOO 9 Pro")) return "Vivo iQOO 9 Pro";
            if (model.contains("V2243A") || model.contains("iQOO 11")) return "Vivo iQOO 11";
            if (model.contains("I2214") || model.contains("iQOO Neo 7")) return "Vivo iQOO Neo 7";
            if (model.contains("V2280") || model.contains("V27 Pro")) return "Vivo V27 Pro";
            if (model.contains("V2250") || model.contains("V29")) return "Vivo V29 5G";
            if (model.contains("V2241A") || model.contains("X90 Pro")) return "Vivo X90 Pro";

            // Infinix & Tecno
            if (model.contains("X6739") || model.contains("GT 10 Pro")) return "Infinix GT 10 Pro";
            if (model.contains("X6710") || model.contains("Note 30 VIP")) return "Infinix Note 30 VIP";
            if (model.contains("X6871") || model.contains("Zero Ultra")) return "Infinix Zero Ultra";
            if (model.contains("CK7n") || model.contains("Camon 20 Pro")) return "Tecno Camon 20 Pro";

            // OnePlus & ASUS
            if (model.contains("CPH2449") || model.contains("OnePlus 11")) return "OnePlus 11";
            if (model.contains("CPH2493") || model.contains("Nord 3")) return "OnePlus Nord 3";
            if (model.contains("ASUS_AI2201") || model.contains("ROG Phone 6")) return "ASUS ROG Phone 6";
            if (model.contains("ASUS_AI2205") || model.contains("ROG Phone 7")) return "ASUS ROG Phone 7";
            if (model.contains("ASUS_AI2401") || model.contains("ROG Phone 8 Pro")) return "ASUS ROG Phone 8 Pro";

            // Google Pixel
            if (model.equalsIgnoreCase("Pixel 8 Pro")) return "Google Pixel 8 Pro";
            if (model.equalsIgnoreCase("Pixel 8")) return "Google Pixel 8";
            if (model.equalsIgnoreCase("Pixel 7")) return "Google Pixel 7";
            if (model.equalsIgnoreCase("Pixel 6")) return "Google Pixel 6";

            // Format manufacturer and model properly
            if (manufacturer.length() > 0) {
                String capMfg = manufacturer.substring(0, 1).toUpperCase() + manufacturer.substring(1).toLowerCase();
                if (model.toLowerCase().startsWith(manufacturer.toLowerCase())) {
                    return model;
                } else {
                    return capMfg + " " + model;
                }
            }
            return model.length() > 0 ? model : "Android Device";
        }

        @JavascriptInterface
        public int getBatteryLevel() {
            IntentFilter ifilter = new IntentFilter(Intent.ACTION_BATTERY_CHANGED);
            Intent batteryStatus = mContext.registerReceiver(null, ifilter);
            if (batteryStatus != null) {
                int level = batteryStatus.getIntExtra(BatteryManager.EXTRA_LEVEL, -1);
                int scale = batteryStatus.getIntExtra(BatteryManager.EXTRA_SCALE, -1);
                if (level >= 0 && scale > 0) {
                    return (int) ((level / (float) scale) * 100);
                }
            }
            return 85;
        }

        @JavascriptInterface
        public String getMemoryInfo() {
            ActivityManager actManager = (ActivityManager) mContext.getSystemService(Context.ACTIVITY_SERVICE);
            ActivityManager.MemoryInfo memInfo = new ActivityManager.MemoryInfo();
            if (actManager != null) {
                actManager.getMemoryInfo(memInfo);
                long totalMb = memInfo.totalMem / (1024 * 1024);
                long availMb = memInfo.availMem / (1024 * 1024);
                long usedMb = totalMb - availMb;
                int percent = (int) (((double) usedMb / totalMb) * 100);
                return "{\"total\":" + totalMb + ",\"used\":" + usedMb + ",\"percent\":" + percent + "}";
            }
            return "{\"total\":7392,\"used\":5278,\"percent\":71}";
        }

        @JavascriptInterface
        public String cleanMemory() {
            try {
                System.gc();
                if (mWebView != null) {
                    mWebView.post(new Runnable() {
                        @Override
                        public void run() {
                            mWebView.clearCache(false);
                        }
                    });
                }
                ActivityManager actManager = (ActivityManager) mContext.getSystemService(Context.ACTIVITY_SERVICE);
                ActivityManager.MemoryInfo memInfo = new ActivityManager.MemoryInfo();
                if (actManager != null) {
                    actManager.getMemoryInfo(memInfo);
                    long totalMb = memInfo.totalMem / (1024 * 1024);
                    long availMb = memInfo.availMem / (1024 * 1024);
                    long usedMb = totalMb - availMb;
                    int percent = (int) (((double) usedMb / totalMb) * 100);
                    return "{\"total\":" + totalMb + ",\"used\":" + usedMb + ",\"percent\":" + percent + "}";
                }
            } catch (Exception e) {}
            return getMemoryInfo();
        }

        @JavascriptInterface
        public String getDisplayTelemetry() {
            try {
                WindowManager wm = (WindowManager) mContext.getSystemService(Context.WINDOW_SERVICE);
                Display display = wm.getDefaultDisplay();
                float refreshRate = display.getRefreshRate();
                DisplayMetrics metrics = new DisplayMetrics();
                display.getRealMetrics(metrics);
                int dpi = metrics.densityDpi;
                int width = metrics.widthPixels;
                int height = metrics.heightPixels;
                int osVersion = Build.VERSION.SDK_INT;
                String release = Build.VERSION.RELEASE;

                return "{\"refreshRate\":" + (int)Math.round(refreshRate) + 
                       ",\"dpi\":" + dpi + 
                       ",\"width\":" + width + 
                       ",\"height\":" + height + 
                       ",\"osVersion\":\"Android " + release + " (API " + osVersion + ")\"}";
            } catch (Exception e) {
                return "{}";
            }
        }

        @JavascriptInterface
        public void triggerHaptic(int milliseconds) {
            Vibrator v = (Vibrator) mContext.getSystemService(Context.VIBRATOR_SERVICE);
            if (v != null) {
                v.vibrate(milliseconds > 0 ? milliseconds : 50);
            }
        }

        @JavascriptInterface
        public String getHWID() {
            try {
                String androidId = Settings.Secure.getString(mContext.getContentResolver(), Settings.Secure.ANDROID_ID);
                if (androidId != null && !androidId.isEmpty()) {
                    return "AND-" + androidId;
                }
            } catch (Exception e) {}
            return "AND-" + Build.SERIAL;
        }

        @JavascriptInterface
        public String getUUID() {
            SharedPreferences prefs = mContext.getSharedPreferences("gk_auth_prefs", Context.MODE_PRIVATE);
            String uuid = prefs.getString("machine_uuid", null);
            if (uuid == null) {
                uuid = UUID.randomUUID().toString();
                prefs.edit().putString("machine_uuid", uuid).apply();
            }
            return uuid;
        }

        @JavascriptInterface
        public String gkAuthGet(String urlStr) {
            try {
                URL url = new URL(urlStr);
                HttpURLConnection conn = (HttpURLConnection) url.openConnection();
                conn.setRequestMethod("GET");
                conn.setRequestProperty("User-Agent", "AloneEngine/3.4");
                conn.setConnectTimeout(10000);
                conn.setReadTimeout(10000);
                conn.setInstanceFollowRedirects(true);
                
                int code = conn.getResponseCode();
                if (code == 301 || code == 302 || code == 303 || code == 307) {
                    String newUrl = conn.getHeaderField("Location");
                    conn.disconnect();
                    return gkAuthGet(newUrl);
                }
                
                BufferedReader reader = new BufferedReader(new InputStreamReader(conn.getInputStream()));
                StringBuilder sb = new StringBuilder();
                String line;
                while ((line = reader.readLine()) != null) {
                    sb.append(line).append("\n");
                }
                reader.close();
                conn.disconnect();
                return sb.toString();
            } catch (Exception e) {
                return "ERROR:" + e.getMessage();
            }
        }

        @JavascriptInterface
        public String gkAuthPost(String urlStr, String jsonBody) {
            try {
                URL url = new URL(urlStr);
                HttpURLConnection conn = (HttpURLConnection) url.openConnection();
                conn.setRequestMethod("POST");
                conn.setRequestProperty("Content-Type", "application/json");
                conn.setRequestProperty("User-Agent", "AloneEngine/3.4");
                conn.setConnectTimeout(12000);
                conn.setReadTimeout(12000);
                conn.setDoOutput(true);
                conn.setInstanceFollowRedirects(true);

                OutputStream os = conn.getOutputStream();
                os.write(jsonBody.getBytes("UTF-8"));
                os.flush();
                os.close();

                int code = conn.getResponseCode();
                if (code == 301 || code == 302 || code == 303 || code == 307) {
                    String newUrl = conn.getHeaderField("Location");
                    conn.disconnect();
                    return gkAuthGet(newUrl);
                }

                InputStream is = (code >= 200 && code < 400) ? conn.getInputStream() : conn.getErrorStream();
                BufferedReader reader = new BufferedReader(new InputStreamReader(is));
                StringBuilder sb = new StringBuilder();
                String line;
                while ((line = reader.readLine()) != null) {
                    sb.append(line);
                }
                reader.close();
                conn.disconnect();
                return sb.toString();
            } catch (Exception e) {
                return "{\"success\":false,\"error\":\"" + e.getMessage() + "\"}";
            }
        }

        @JavascriptInterface
        public boolean savePermanentSetting(String key, String value) {
            if (key == null || key.trim().isEmpty() || value == null) {
                return false;
            }
            try {
                // Validate key format: letters, numbers, underscores, dashes, dots
                if (!key.matches("^[a-zA-Z0-9_\\-\\.]+$")) {
                    return false;
                }
                SharedPreferences prefs = mContext.getSharedPreferences("sensi_permanent_config", Context.MODE_PRIVATE);
                // Synchronous commit() ensures atomic write to disk that survives phone restart
                return prefs.edit().putString(key.trim(), value).commit();
            } catch (Exception e) {
                return false;
            }
        }

        @JavascriptInterface
        public String getPermanentSetting(String key, String defaultValue) {
            if (key == null || key.trim().isEmpty()) {
                return defaultValue;
            }
            try {
                SharedPreferences prefs = mContext.getSharedPreferences("sensi_permanent_config", Context.MODE_PRIVATE);
                return prefs.getString(key.trim(), defaultValue);
            } catch (Exception e) {
                return defaultValue;
            }
        }

        @JavascriptInterface
        public boolean removePermanentSetting(String key) {
            if (key == null || key.trim().isEmpty()) return false;
            try {
                SharedPreferences prefs = mContext.getSharedPreferences("sensi_permanent_config", Context.MODE_PRIVATE);
                return prefs.edit().remove(key.trim()).commit();
            } catch (Exception e) {
                return false;
            }
        }

        @JavascriptInterface
        public boolean clearAllPermanentSettings() {
            try {
                SharedPreferences prefs = mContext.getSharedPreferences("sensi_permanent_config", Context.MODE_PRIVATE);
                return prefs.edit().clear().commit();
            } catch (Exception e) {
                return false;
            }
        }

        @JavascriptInterface
        public String detectFreeFireInstallations() {
            PackageManager pm = mContext.getPackageManager();
            boolean stdInstalled = false;
            boolean maxInstalled = false;
            try {
                pm.getPackageInfo("com.dts.freefireth", 0);
                stdInstalled = true;
            } catch (Exception e) {}
            try {
                pm.getPackageInfo("com.dts.freefiremax", 0);
                maxInstalled = true;
            } catch (Exception e) {}

            return "{\"standard\":" + stdInstalled + ",\"max\":" + maxInstalled + "}";
        }

        @JavascriptInterface
        public boolean launchGameTarget(String target) {
            PackageManager pm = mContext.getPackageManager();
            String targetPkg = null;
            if ("max".equalsIgnoreCase(target) || "com.dts.freefiremax".equalsIgnoreCase(target)) {
                targetPkg = "com.dts.freefiremax";
            } else if ("standard".equalsIgnoreCase(target) || "com.dts.freefireth".equalsIgnoreCase(target)) {
                targetPkg = "com.dts.freefireth";
            } else {
                // Auto-detect: prioritize MAX if installed, else standard
                if (pm.getLaunchIntentForPackage("com.dts.freefiremax") != null) {
                    targetPkg = "com.dts.freefiremax";
                } else if (pm.getLaunchIntentForPackage("com.dts.freefireth") != null) {
                    targetPkg = "com.dts.freefireth";
                }
            }

            if (targetPkg != null) {
                Intent launchIntent = pm.getLaunchIntentForPackage(targetPkg);
                if (launchIntent != null) {
                    launchIntent.addFlags(Intent.FLAG_ACTIVITY_NEW_TASK);
                    mContext.startActivity(launchIntent);
                    return true;
                }
            }

            final String missingTarget = ("com.dts.freefiremax".equals(targetPkg)) ? "Free Fire MAX" :
                                         ("com.dts.freefireth".equals(targetPkg)) ? "Free Fire" : "Free Fire / Free Fire MAX";
            ((Activity) mContext).runOnUiThread(new Runnable() {
                @Override
                public void run() {
                    Toast.makeText(mContext, missingTarget + " is not installed on this device.", Toast.LENGTH_SHORT).show();
                }
            });
            return false;
        }

        @JavascriptInterface
        public void launchGame() {
            launchGameTarget("auto");
        }

        @JavascriptInterface
        public boolean canDrawOverlays() {
            if (Build.VERSION.SDK_INT >= Build.VERSION_CODES.M) {
                return Settings.canDrawOverlays(mContext);
            }
            return true;
        }

        @JavascriptInterface
        public void requestOverlayPermission() {
            if (Build.VERSION.SDK_INT >= Build.VERSION_CODES.M) {
                if (!Settings.canDrawOverlays(mContext)) {
                    ((Activity) mContext).runOnUiThread(new Runnable() {
                        @Override
                        public void run() {
                            try {
                                Intent intent = new Intent(
                                    Settings.ACTION_MANAGE_OVERLAY_PERMISSION,
                                    android.net.Uri.parse("package:" + mContext.getPackageName())
                                );
                                intent.addFlags(Intent.FLAG_ACTIVITY_NEW_TASK);
                                mContext.startActivity(intent);
                                Toast.makeText(mContext, "Please allow 'Display over other apps' for SENSI", Toast.LENGTH_LONG).show();
                            } catch (Exception e) {
                                try {
                                    Intent genericIntent = new Intent(Settings.ACTION_MANAGE_OVERLAY_PERMISSION);
                                    genericIntent.addFlags(Intent.FLAG_ACTIVITY_NEW_TASK);
                                    mContext.startActivity(genericIntent);
                                } catch (Exception ex) {
                                    Toast.makeText(mContext, "Could not open overlay settings: " + ex.getMessage(), Toast.LENGTH_SHORT).show();
                                }
                            }
                        }
                    });
                }
            }
        }

        @JavascriptInterface
        public boolean startFloatingGuide() {
            if (Build.VERSION.SDK_INT >= Build.VERSION_CODES.M && !Settings.canDrawOverlays(mContext)) {
                requestOverlayPermission();
                return false;
            }
            try {
                Intent serviceIntent = new Intent(mContext, FloatingGuideService.class);
                serviceIntent.setAction(FloatingGuideService.ACTION_START);
                mContext.startService(serviceIntent);
                return true;
            } catch (Exception e) {
                return false;
            }
        }

        @JavascriptInterface
        public void stopFloatingGuide() {
            try {
                Intent serviceIntent = new Intent(mContext, FloatingGuideService.class);
                serviceIntent.setAction(FloatingGuideService.ACTION_STOP);
                mContext.startService(serviceIntent);
            } catch (Exception e) {}
        }

        @JavascriptInterface
        public boolean isFloatingGuideRunning() {
            return FloatingGuideService.isRunning;
        }

        @JavascriptInterface
        public void updateFloatingGuide() {
            try {
                Intent serviceIntent = new Intent(mContext, FloatingGuideService.class);
                serviceIntent.setAction(FloatingGuideService.ACTION_UPDATE);
                mContext.startService(serviceIntent);
            } catch (Exception e) {}
        }

        // =====================================================================
        // NATIVE AIMHUD & CROSSHAIR OVERLAY BRIDGE
        // =====================================================================
        @JavascriptInterface
        public boolean startAimHud(final String configJson) {
            if (Build.VERSION.SDK_INT >= Build.VERSION_CODES.M && !Settings.canDrawOverlays(mContext)) {
                requestOverlayPermission();
                return false;
            }
            try {
                Intent serviceIntent = new Intent(mContext, AimHudService.class);
                serviceIntent.setAction(AimHudService.ACTION_START);
                if (configJson != null) {
                    serviceIntent.putExtra(AimHudService.EXTRA_CONFIG_JSON, configJson);
                }
                if (Build.VERSION.SDK_INT >= Build.VERSION_CODES.O) {
                    mContext.startForegroundService(serviceIntent);
                } else {
                    mContext.startService(serviceIntent);
                }
                return true;
            } catch (Exception e) {
                return false;
            }
        }

        @JavascriptInterface
        public void updateAimHud(final String configJson) {
            try {
                Intent serviceIntent = new Intent(mContext, AimHudService.class);
                serviceIntent.setAction(AimHudService.ACTION_UPDATE);
                if (configJson != null) {
                    serviceIntent.putExtra(AimHudService.EXTRA_CONFIG_JSON, configJson);
                }
                mContext.startService(serviceIntent);
            } catch (Exception e) {}
        }

        @JavascriptInterface
        public void stopAimHud() {
            try {
                Intent serviceIntent = new Intent(mContext, AimHudService.class);
                serviceIntent.setAction(AimHudService.ACTION_STOP);
                mContext.startService(serviceIntent);
            } catch (Exception e) {}
        }

        @JavascriptInterface
        public boolean isAimHudRunning() {
            return AimHudService.isRunning;
        }

        @JavascriptInterface
        public void setAimHudLock(boolean locked) {
            try {
                Intent serviceIntent = new Intent(mContext, AimHudService.class);
                serviceIntent.setAction(locked ? AimHudService.ACTION_LOCK : AimHudService.ACTION_UNLOCK);
                mContext.startService(serviceIntent);
            } catch (Exception e) {}
        }

        @JavascriptInterface
        public boolean isAimHudLocked() {
            return AimHudService.isLocked;
        }

        @JavascriptInterface
        public void resetAimHudPosition() {
            try {
                Intent serviceIntent = new Intent(mContext, AimHudService.class);
                serviceIntent.setAction(AimHudService.ACTION_RESET_POS);
                mContext.startService(serviceIntent);
            } catch (Exception e) {}
        }

        // =====================================================================
        // DEDICATED MACRO OVERLAY BRIDGE
        // =====================================================================
        @JavascriptInterface
        public boolean applyMacroOverlay(final String configJson) {
            if (Build.VERSION.SDK_INT >= Build.VERSION_CODES.M && !Settings.canDrawOverlays(mContext)) {
                requestOverlayPermission();
                return false;
            }
            try {
                Intent serviceIntent = new Intent(mContext, AimHudService.class);
                serviceIntent.setAction(AimHudService.ACTION_APPLY_MACRO);
                if (configJson != null) {
                    serviceIntent.putExtra(AimHudService.EXTRA_CONFIG_JSON, configJson);
                }
                if (Build.VERSION.SDK_INT >= Build.VERSION_CODES.O) {
                    mContext.startForegroundService(serviceIntent);
                } else {
                    mContext.startService(serviceIntent);
                }
                return true;
            } catch (Exception e) {
                return false;
            }
        }

        @JavascriptInterface
        public void removeMacroOverlay() {
            try {
                Intent serviceIntent = new Intent(mContext, AimHudService.class);
                serviceIntent.setAction(AimHudService.ACTION_REMOVE_MACRO);
                mContext.startService(serviceIntent);
            } catch (Exception e) {}
        }

        @JavascriptInterface
        public void setMacroLock(boolean locked) {
            try {
                Intent serviceIntent = new Intent(mContext, AimHudService.class);
                serviceIntent.setAction(locked ? AimHudService.ACTION_LOCK_MACRO : AimHudService.ACTION_UNLOCK_MACRO);
                mContext.startService(serviceIntent);
            } catch (Exception e) {}
        }

        @JavascriptInterface
        public boolean isMacroLocked() {
            return AimHudService.isMacroLocked;
        }

        @JavascriptInterface
        public void resetMacroPosition() {
            try {
                Intent serviceIntent = new Intent(mContext, AimHudService.class);
                serviceIntent.setAction(AimHudService.ACTION_RESET_MACRO);
                mContext.startService(serviceIntent);
            } catch (Exception e) {}
        }

        @JavascriptInterface
        public String getAimHudConfig() {
            SharedPreferences prefs = mContext.getSharedPreferences("sensi_aimhud_prefs", Context.MODE_PRIVATE);
            org.json.JSONObject obj = new org.json.JSONObject();
            try {
                obj.put("style", prefs.getString("style", "plus"));
                obj.put("size", prefs.getInt("size", 36));
                obj.put("opacity", (int)(prefs.getFloat("opacity", 0.85f) * 100));
                obj.put("rotation", (int)prefs.getFloat("rotation", 0f));
                int color = prefs.getInt("color", Color.parseColor("#00E5FF"));
                obj.put("color", String.format("#%06X", (0xFFFFFF & color)));
                obj.put("posX", prefs.getInt("pos_x", 0));
                obj.put("posY", prefs.getInt("pos_y", 0));
                obj.put("locked", prefs.getBoolean("locked", true));
                obj.put("running", AimHudService.isRunning);
                obj.put("showMacro", prefs.getBoolean("show_macro", false));
                obj.put("macroFireSize", prefs.getInt("macro_fire_size", 48));
                obj.put("macroOpacity", (int)(prefs.getFloat("macro_opacity", 0.85f) * 100));
                int mcol = prefs.getInt("macro_color", Color.parseColor("#00E676"));
                obj.put("macroColor", String.format("#%06X", (0xFFFFFF & mcol)));
                obj.put("macroFireX", prefs.getInt("macro_fire_x", 140));
                obj.put("macroFireY", prefs.getInt("macro_fire_y", 160));
                obj.put("macroLocked", prefs.getBoolean("macro_locked", true));
                return obj.toString();
            } catch (Exception e) {
                return "{}";
            }
        }

        @JavascriptInterface
        public void openUrl(String url) {
            try {
                Intent intent = new Intent(Intent.ACTION_VIEW, android.net.Uri.parse(url));
                intent.addFlags(Intent.FLAG_ACTIVITY_NEW_TASK);
                mContext.startActivity(intent);
            } catch (Exception e) {
                final String err = e.getMessage();
                ((Activity) mContext).runOnUiThread(new Runnable() {
                    @Override
                    public void run() {
                        Toast.makeText(mContext, "Could not open URL: " + err, Toast.LENGTH_SHORT).show();
                    }
                });
            }
        }

        @JavascriptInterface
        public boolean copyToClipboard(final String text) {
            if (text == null) return false;
            try {
                ((Activity) mContext).runOnUiThread(new Runnable() {
                    @Override
                    public void run() {
                        try {
                            android.content.ClipboardManager clipboard = (android.content.ClipboardManager) mContext.getSystemService(Context.CLIPBOARD_SERVICE);
                            android.content.ClipData clip = android.content.ClipData.newPlainText("Sensi Settings", text);
                            clipboard.setPrimaryClip(clip);
                        } catch (Exception ex) {}
                    }
                });
                return true;
            } catch (Exception e) {
                return false;
            }
        }
    }

    @Override
    public void onBackPressed() {
        if (mWebView != null && mWebView.canGoBack()) {
            mWebView.goBack();
        } else {
            super.onBackPressed();
        }
    }

    @Override
    protected void onDestroy() {
        super.onDestroy();
        // Clean up floating service only if activity is explicitly finishing (user explicitly backed out/exited)
        // If destroyed by Android OS memory management while user is in Free Fire, keep the overlay active!
        try {
            if (isFinishing()) {
                Intent serviceIntent = new Intent(this, FloatingGuideService.class);
                serviceIntent.setAction(FloatingGuideService.ACTION_STOP);
                stopService(serviceIntent);
            }
        } catch (Exception e) {}
    }
}
