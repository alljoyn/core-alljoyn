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
import org.gradle.api.plugins.quality.FindBugs

class Findbugs implements Plugin<Project> {
    void apply(Project project) {
        project.logger.info("Applying alljoyn-android-findbugs plugin")

        //findbugs
        project.task(["type": FindBugs], "findbugsMain") {
            description("Run findbugs on main sources. Add \"-pfindbugsHtml=true\" to create html report")
            dependsOn("compileDebugJava")
            group "Verification"
            project.android.sourceSets.main.java.srcDirs.each { srcdir ->
                source srcdir
            }
            FileTree tree = project.fileTree(dir: "${project.buildDir}/intermediates/classes/debug")
            if (project.hasProperty("findbugsExclude")) {
                project.findbugsExclude.each {
                    tree.exclude it
                }
            }
            tree.exclude '**/R.class' //exclude generated R.java
            tree.exclude '**/R$*.class' //exclude generated R.java inner classes
            classes = tree


            classpath = project.files("${System.env.ANDROID_HOME}/platforms/${project.android.compileSdkVersion}/android.jar")
            project.afterEvaluate {
                for  (i in project.configurations.compile) {
                    if (i.getName().endsWith(".jar")) {
                        //regular jar files
                        logger.debug("added ${i} to findbugsMain classpath")
                        classpath += project.files(i)
                    }
                }

                //find all classes.jar coming from aar bundles
                FileTree treeAar = project.fileTree(dir: "${project.buildDir}/intermediates/exploded-aar")
                treeAar.include '*/*/*/classes.jar'
                treeAar.each {
                    classpath += project.files(it)
                    logger.debug("added ${it} to findbugsMain classpath")
                }

            }

            if (project.hasProperty("findbugsHtml") && project.findbugsHtml) {
                //enable readable findbugs reports, you can't do both
                logger.warn("Creating findbugs HTML instead of XML")
                reports.html.enabled = true
                reports.xml.enabled = false
            }
        }
        project.tasks.check.dependsOn("findbugsMain")

    }

}

