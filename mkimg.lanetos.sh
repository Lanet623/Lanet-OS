profile_lanetos() {
    profile_standard
    profile_abbrev="lanet"
    image_name="lanetos"
    title="Lanet OS"
    desc="Alpine Linux with Lanet Shell and Lanet Control Center."
    apkovl="genapkovl-lanetos.sh"
    hostname="lanetos"
    apks="$apks lanet-shell"
}
