package org.alljoyn

import org.gradle.api.Project
import org.gradle.api.Plugin
import org.gradle.api.tasks.Exec
import org.gradle.api.tasks.Copy
import org.gradle.api.tasks.bundling.Jar
import org.gradle.api.tasks.bundling.Zip
import org.gradle.api.InvalidUserDataException
import org.gradle.api.file.FileCollection
import org.gradle.api.file.FileTree
import org.gradle.api.GradleException
import org.gradle.api.JavaVersion 

class AlljoynAndroid implements Plugin<Project> {
    void apply(Project project) {
        project.logger.info("Applying alljoyn-android plugin")

        File buildDirRoot = project.rootProject.file("../../build")
        File buildDirArmDebug = new File(buildDirRoot, "android/arm/debug/dist/security")
        File buildDirX86Debug = new File(buildDirRoot, "android/x86/debug/dist/security")
        File buildDirArmRelease = new File(buildDirRoot, "android/arm/release/dist/security")

        File soFileArmDebug = new File(buildDirArmDebug, "lib/libajsecmgr_java.so")
        File soFileX86Debug = new File(buildDirX86Debug, "lib/libajsecmgr_java.so")
        File soFilesDebug = new File(project.buildDir, "soFiles-debug")
        project.task(["type": Copy], "copySoFilesDebug") {
            destinationDir soFilesDebug
            into ("armeabi") {
                from soFileArmDebug
            }
            into ("x86") {
                from soFileX86Debug
            }
        }
        
        File soFilesRelease = new File(project.buildDir, "soFiles-release")
        File soFileRelease = new File(buildDirArmRelease, "lib/libajsecmgr_java.so")
        project.task(["type": Copy], "copySoFilesRelease") {
            destinationDir soFilesRelease
            into ("armeabi") {
                from soFileRelease
            }
        }

        project.afterEvaluate {
            project.tasks.packageDebug.dependsOn("copySoFilesDebug")
            project.tasks.packageDebug {
                doFirst {
                    if (!soFileArmDebug.exists()) {
                        throw new GradleException("Can't find ${soFileArmDebug}. Did you run scons for \"BINDINGS=java OS=android CPU=arm\" first?")
                    }
                }
            }
            project.tasks.packageRelease.dependsOn("copySoFilesRelease")
            project.tasks.packageRelease {
                doFirst {
                    if (!soFileRelease.exists()) {
                        throw new GradleException("Can't find ${soFileRelease}. Did you run scons for \"BINDINGS=java OS=android CPU=arm VARIANT=release\" first?")
                    }
                }
            }
        }

        //Using the jarfile from android/arm/debug. Does not matter which one to use...
        File jarAlljoyn = new File(buildDirArmDebug, "jar/secmgr.jar")
        if (!jarAlljoyn.exists()) {
            throw new GradleException("Can't find ${jarAlljoyn}. Did you run scons for \"BINDINGS=java OS=android CPU=arm\" first?")
        }

        project.dependencies {
            compile project.files(jarAlljoyn)
        }

        project.android {
            sourceSets.debug {
                jniLibs {
                    srcDir soFilesDebug
                }
            }
            sourceSets.release {
                jniLibs {
                    srcDir soFilesRelease
                }
            }

            buildTypes {
                release {
                    //sign release build also with debug key
                    //This should be resigned with a proper secured key afterwards
                    signingConfig = project.android.signingConfigs.debug
                    minifyEnabled = true
                    shrinkResources = true
                    proguardFile getDefaultProguardFile('proguard-android-optimize.txt')
                    proguardFile 'proguard-rules.pro'
                }
            }
            compileOptions {
                //enable Java7 support
                sourceCompatibility JavaVersion.VERSION_1_7
                targetCompatibility JavaVersion.VERSION_1_7
            }

            lintOptions {
                // set to true to turn off analysis progress reporting by lint
                quiet false
                // if true, stop the gradle build if errors are found
                abortOnError true
                // if true, only report errors
                ignoreWarnings false
                // if true, emit full/absolute paths to files with errors (true by default)
                //absolutePaths true
                // if true, check all issues, including those that are off by default
                checkAllWarnings true
                // if true, treat all warnings as errors
                warningsAsErrors false
                // turn off checking the given issue id's
                //disable 'TypographyFractions','TypographyQuotes'
                // turn on the given issue id's
                //enable 'RtlHardcoded','RtlCompat', 'RtlEnabled'
                // check *only* the given issue id's
                //check 'NewApi', 'InlinedApi'
                // if true, don't include source code lines in the error output
                //noLines true
                // if true, show all locations for an error, do not truncate lists, etc.
                showAll true
                // Fallback lint configuration (default severities, etc.)
                //lintConfig file("default-lint.xml")
                // if true, generate a text report of issues (false by default)
                textReport false
                // location to write the output; can be a file or 'stdout'
                textOutput 'stdout'
                // if true, generate an XML report for use by for example Jenkins
                xmlReport true
                // if true, generate an HTML report (with issue explanations, sourcecode, etc)
                htmlReport true
            }
        }
    }

}

