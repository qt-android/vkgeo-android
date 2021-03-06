package com.derevenetz.oleg.vkgeo.gplay;

import android.content.BroadcastReceiver;
import android.content.Context;
import android.content.Intent;
import android.os.Build;

public class VKGeoReceiver extends BroadcastReceiver
{
    @Override
    public void onReceive(Context context, Intent intent)
    {
        if (intent.getAction().equals(Intent.ACTION_BOOT_COMPLETED) ||
            intent.getAction().equals(Intent.ACTION_MY_PACKAGE_REPLACED)) {
            if (Build.VERSION.SDK_INT >= Build.VERSION_CODES.O) {
                context.startForegroundService(new Intent(context, VKGeoService.class));
            } else {
                context.startService(new Intent(context, VKGeoService.class));
            }
        }
    }
}
