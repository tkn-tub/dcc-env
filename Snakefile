import glob

rule all:
    input: ["src/experiment_dbg", "src/experiment"]

rule protobuf:
    input: "src/protobuf/{file}.proto"
    output:
        cpp=multiext("src/protobuf/{file}", ".pb.cc", ".pb.h"),
    shell: "env protoc --proto_path src/protobuf --cpp_out src/protobuf {input}"

rule configure:
    input:
        code_files=[glob.glob(f"src/**/*.{ext}", recursive=True) for ext in ["msg", "cc", "h"]],
        cpp=multiext("src/protobuf/veinsgym.pb.", "cc", "h"),
    output: "src/Makefile"
    params:
        include_flags = ' '.join(['-I.', '-I../lib/veins/src']),
        link_flags = ' '.join(['-L../lib/veins/src/', '-lveins\\$\(D\)', '-lzmq', '-lprotobuf']),
        flags = ' '.join(['-f', '--deep', '-o', 'experiment', '-O', 'out']),
    shell: "env -C src opp_makemake {params.flags} {params.include_flags} {params.link_flags}"

rule configure_veins:
    input: [glob.glob(f"lib/veins/src/**/*.{ext}", recursive=True) for ext in ["msg", "cc", "h"]]
    output: "lib/veins/src/Makefile"
    shell: "env -C lib/veins ./configure"

rule build_veins:
    input: "lib/veins/src/Makefile",
    output: "lib/veins/src/libveins{dbg,(_dbg)?}.so"
    params: mode=lambda wildcards, output: "debug" if "_dbg" == wildcards.dbg else "release"
    threads: workflow.cores
    shell: "make -j{threads} -C lib/veins MODE={params.mode}"

rule build:
    input: rules.build_veins.output, "src/Makefile"
    output: "src/experiment{dbg,(_dbg)?}"
    params: mode=lambda wildcards, output: "debug" if "_dbg" == wildcards.dbg else "release"
    threads: workflow.cores
    shell: "make -j{threads} -C src MODE={params.mode}"
