package com.amazon.apl.android;

import org.gradle.api.Action;
import org.gradle.api.DefaultTask;
import org.gradle.api.Task;
import org.gradle.api.file.RegularFile;
import org.gradle.api.plugins.JavaPluginExtension;
import org.gradle.api.provider.Property;
import org.gradle.api.provider.Provider;
import org.gradle.api.tasks.TaskAction;
import org.gradle.api.tasks.OutputDirectory;
import org.gradle.api.file.RegularFileProperty;
import org.gradle.api.tasks.compile.JavaCompile;
import org.gradle.jvm.toolchain.JavaCompiler;
import org.gradle.jvm.toolchain.JavaLanguageVersion;
import org.gradle.jvm.toolchain.JavaLauncher;
import org.gradle.jvm.toolchain.JavaToolchainService;
import org.gradle.jvm.toolchain.JavaToolchainSpec;
import org.gradle.jvm.toolchain.JvmImplementation;
import org.gradle.jvm.toolchain.JvmVendorSpec;
import org.gradle.process.ExecOperations;

import java.io.File;
import java.util.ArrayList;
import java.util.List;
import java.util.Set;

import javax.inject.Inject;

class CMakeTask extends DefaultTask {
    private final ExecOperations execOperations;
    private List<String> cmakeArgs = new ArrayList<>();
    private List<String> makeTargets = new ArrayList<>();

    private RegularFile buildDirectory;

    @OutputDirectory
    public RegularFileProperty getBuildFolder() {
        return getProject().getObjects().fileProperty()
                .convention(buildDirectory);
    }

    @Inject
    public CMakeTask(ExecOperations execOperations) {
        this.execOperations = execOperations;
        cmakeArgs.add("../../../../");

        buildDirectory = getProject().getLayout().getProjectDirectory().file(".cxx/cmake/debug/host/");
    }

    public void cmakeArgs(String... args) {
        cmakeArgs.addAll(List.of(args));
    }

    public void makeTargets(String... targets) {
        makeTargets.addAll(List.of(targets));
    }

    @TaskAction
    public void run() {
        String hostDir = buildDirectory.getAsFile().getAbsolutePath();

        File folder = new File(hostDir);
        if (!folder.exists()) {
            folder.mkdirs();
        }

        // Find a java 17 jdk. The embedded one in Android Studio doesn't include JNI.
        JavaToolchainService service = getProject().getExtensions().getByType(JavaToolchainService.class);
        Provider<JavaCompiler> compiler = service.compilerFor(javaToolchainSpec -> javaToolchainSpec.getLanguageVersion().set(JavaLanguageVersion.of(17)));

        if (!compiler.isPresent()) {
            throw new RuntimeException("Unable to find a suitable jdk on the system.");
        }

        String jdkPath = compiler.get().getMetadata().getInstallationPath().getAsFile().getAbsolutePath();

        getProject().getLogger().error(jdkPath);

        execOperations.exec(execSpec -> {
            execSpec.workingDir(hostDir);
            execSpec.commandLine("cmake");
            execSpec.args(cmakeArgs);
            execSpec.environment("JAVA_HOME", jdkPath);
        });

        execOperations.exec(execSpec -> {
            execSpec.workingDir(hostDir);
            execSpec.commandLine("make");
            execSpec.args(makeTargets);
            execSpec.environment("JAVA_HOME", jdkPath);
        });
    }
}

