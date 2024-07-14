package org.thecodedeposit.aerofoil;

import org.libsdl.app.SDLActivity;

import android.content.ContentResolver;
import android.content.ContentValues;
import android.content.Context;
import android.content.Intent;
import android.content.res.AssetManager;
import android.net.Uri;
import android.os.Bundle;
import android.os.Environment;
import android.os.ParcelFileDescriptor;
import android.provider.MediaStore;

import java.io.FileNotFoundException;
import java.io.IOException;
import java.io.OutputStream;

public class GpActivity extends SDLActivity
{
    private static final int SOURCE_EXPORT_REQUEST_ID = 20;

    private AssetManager assetManager;

    @Override
    protected void onCreate(Bundle savedInstanceState)
    {
        super.onCreate(savedInstanceState);

        assetManager = getAssets();
    }

    public String[] scanAssetDirectory(String directory)
    {
        try
        {
            return this.assetManager.list(directory);
        }
        catch (java.io.IOException ex)
        {
            return new String[0];
        }
    }

    @Override
    protected void onActivityResult(int requestCode, int resultCode, Intent intent)
    {
        if (requestCode == SOURCE_EXPORT_REQUEST_ID)
        {
            if (resultCode == RESULT_OK)
            {
                Uri uri = intent.getData();
                Context context = getContext();
                ContentResolver contentResolver = context.getContentResolver();
                try
                {
                    ParcelFileDescriptor fd = contentResolver.openFileDescriptor(uri, "w");
                    GpFileSystemAPI.nativePostSourceExportRequest(false, fd.getFd(), fd);
                }
                catch (IOException e)
                {
                    GpFileSystemAPI.nativePostSourceExportRequest(true, 0, null);
                    return;
                }
                catch (Exception e)
                {
                    GpFileSystemAPI.nativePostSourceExportRequest(true, 0, null);
                    return;
                }
            }
            else
                GpFileSystemAPI.nativePostSourceExportRequest(true, 0, null);
        }
        else
        {
            super.onActivityResult(requestCode, resultCode, intent);
        }
    }

    public void selectSourceExportPath(String fname)
    {
        Intent intent = new Intent(Intent.ACTION_CREATE_DOCUMENT)
                .setType("application/zip")
                .addCategory(Intent.CATEGORY_OPENABLE)
                .putExtra(Intent.EXTRA_TITLE, fname);
        startActivityForResult(intent, SOURCE_EXPORT_REQUEST_ID);
    }

    public void closeSourceExportPFD(Object obj)
    {
        try
        {
            ((ParcelFileDescriptor) obj).close();
        }
        catch (IOException e)
        {
        }
    }
}
