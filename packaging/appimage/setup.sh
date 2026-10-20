
### BEGIN INKSCAPE-SPECIFIC SETUP

# Note: Only put logic in here that absolutely cannot be done at AppImage build time.
#       Most setup logic belongs in generate.sh instead.

# Glycin find its loader executables by querying config files that hold hardcoded absolute paths to the loaders.
# It does not support launching via relative paths (it uses some extra bwrap logic that gets in the way).
# Thus we need to manually tell it where to find its loaders at runtime, because AppImages mounts are dynamic.
# We can't edit the config files inside $HERE, so we'll first copy them to a temporary location.
export GLYCIN_DATA_DIR=$(mktemp -d)
_CONF_DIR="$GLYCIN_DATA_DIR/glycin-loaders/2+/conf.d"
mkdir -p "$_CONF_DIR"
cp "$HERE"/usr/share/glycin-loaders/2+/conf.d/* "$_CONF_DIR"
sed -i "s|Exec=/usr/libexec/glycin-loaders/|Exec=$HERE/usr/libexec/glycin-loaders/|g" "$_CONF_DIR"/*.conf

### END INKSCAPE-SPECIFIC SETUP
