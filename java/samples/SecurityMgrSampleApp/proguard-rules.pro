# Add project specific ProGuard rules here.
# By default, the flags in this file are appended to flags specified
# in /home/users/cpeqeo/tools/android-sdk-linux/tools/proguard/proguard-android.txt
# You can edit the include path and order by changing the proguardFiles
# directive in build.gradle.
#
# For more details, see
#   http://developer.android.com/guide/developing/tools/proguard.html

# Add any project specific keep options here:

# do not proguard any of the alljoyn classes since they use jni everywhere.
-keep class org.alljoyn.** { *; }

