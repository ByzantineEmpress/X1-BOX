package com.izzy2lost.x1box

import android.app.Activity
import android.content.Context
import android.content.Intent
import android.net.Uri
import android.os.Build
import android.os.Handler
import android.os.Looper
import android.util.Log
import android.widget.Toast
import androidx.core.content.FileProvider
import com.google.android.material.dialog.MaterialAlertDialogBuilder
import org.json.JSONObject
import java.io.File
import java.io.FileOutputStream
import java.net.HttpURLConnection
import java.net.URL
import java.util.concurrent.Executors

object AppUpdater {
  private const val TAG = "AppUpdater"
  private const val RELEASES_API_URL = "https://api.github.com/repos/ByzantineEmpress/X1-BOX/releases/latest"
  private val executor = Executors.newSingleThreadExecutor()
  private val mainHandler = Handler(Looper.getMainLooper())

  data class ReleaseInfo(
    val tagName: String,
    val title: String,
    val notes: String,
    val downloadUrl: String
  )

  fun checkForUpdates(activity: Activity, silentIfLatest: Boolean = true) {
    executor.execute {
      try {
        val url = URL(RELEASES_API_URL)
        val conn = url.openConnection() as HttpURLConnection
        conn.requestMethod = "GET"
        conn.setRequestProperty("User-Agent", "X1-BOX-AndroidApp")
        conn.connectTimeout = 8000
        conn.readTimeout = 8000

        if (conn.responseCode != 200) {
          if (!silentIfLatest) {
            mainHandler.post {
              Toast.makeText(activity, "Failed to check for updates (HTTP ${conn.responseCode})", Toast.LENGTH_SHORT).show()
            }
          }
          return@execute
        }

        val jsonStr = conn.inputStream.bufferedReader().use { it.readText() }
        val root = JSONObject(jsonStr)
        val tagName = root.optString("tag_name", "")
        val title = root.optString("name", tagName)
        val notes = root.optString("body", "Bug fixes & improvements.")
        
        var downloadUrl = ""
        val assets = root.optJSONArray("assets")
        if (assets != null) {
          for (i in 0 until assets.length()) {
            val asset = assets.getJSONObject(i)
            val name = asset.optString("name", "")
            if (name.endsWith(".apk")) {
              downloadUrl = asset.optString("browser_download_url", "")
              break
            }
          }
        }

        if (downloadUrl.isEmpty()) {
          if (!silentIfLatest) {
            mainHandler.post {
              Toast.makeText(activity, "No APK asset found in latest release", Toast.LENGTH_SHORT).show()
            }
          }
          return@execute
        }

        val release = ReleaseInfo(tagName, title, notes, downloadUrl)
        mainHandler.post {
          promptUpdateIfNewer(activity, release, silentIfLatest)
        }
      } catch (e: Exception) {
        Log.e(TAG, "Update check failed", e)
        if (!silentIfLatest) {
          mainHandler.post {
            Toast.makeText(activity, "Error checking for updates: ${e.message}", Toast.LENGTH_SHORT).show()
          }
        }
      }
    }
  }

  private fun promptUpdateIfNewer(activity: Activity, release: ReleaseInfo, silentIfLatest: Boolean) {
    if (activity.isFinishing || activity.isDestroyed) return

    val currentVersion = try {
      activity.packageManager.getPackageInfo(activity.packageName, 0).versionName ?: "0.0.0"
    } catch (e: Exception) {
      "0.0.0"
    }

    if (release.tagName == currentVersion || currentVersion.startsWith(release.tagName)) {
      if (!silentIfLatest) {
        Toast.makeText(activity, "X1-BOX is up to date ($currentVersion)", Toast.LENGTH_SHORT).show()
      }
      return
    }

    MaterialAlertDialogBuilder(activity)
      .setTitle("🚀 Update Available: ${release.tagName}")
      .setMessage("${release.title}\n\n${release.notes.take(500)}")
      .setPositiveButton("Download & Update") { _, _ ->
        downloadAndInstallApk(activity, release)
      }
      .setNegativeButton("Later", null)
      .show()
  }

  private fun downloadAndInstallApk(activity: Activity, release: ReleaseInfo) {
    val progressDialog = MaterialAlertDialogBuilder(activity)
      .setTitle("Downloading Update...")
      .setMessage("Please wait while the update is downloaded.")
      .setCancelable(false)
      .show()

    executor.execute {
      try {
        val url = URL(release.downloadUrl)
        val conn = url.openConnection() as HttpURLConnection
        conn.connectTimeout = 15000
        conn.readTimeout = 15000
        conn.connect()

        val apkFile = File(activity.cacheDir, "update.apk")
        if (apkFile.exists()) apkFile.delete()

        conn.inputStream.use { input ->
          FileOutputStream(apkFile).use { output ->
            input.copyTo(output)
          }
        }

        mainHandler.post {
          progressDialog.dismiss()
          installApk(activity, apkFile)
        }
      } catch (e: Exception) {
        Log.e(TAG, "Failed to download update APK", e)
        mainHandler.post {
          progressDialog.dismiss()
          Toast.makeText(activity, "Download failed: ${e.message}", Toast.LENGTH_LONG).show()
        }
      }
    }
  }

  fun installApk(context: Context, apkFile: File) {
    try {
      val authority = "${context.packageName}.fileprovider"
      val apkUri: Uri = FileProvider.getUriForFile(context, authority, apkFile)
      val intent = Intent(Intent.ACTION_VIEW).apply {
        setDataAndType(apkUri, "application/vnd.android.package-archive")
        addFlags(Intent.FLAG_GRANT_READ_URI_PERMISSION)
        addFlags(Intent.FLAG_ACTIVITY_NEW_TASK)
      }
      context.startActivity(intent)
    } catch (e: Exception) {
      Log.e(TAG, "Failed to launch package installer", e)
      Toast.makeText(context, "Failed to launch installer: ${e.message}", Toast.LENGTH_LONG).show()
    }
  }
}
