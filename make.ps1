# Run GNU Make in a POSIX shell (sh, bash, etc.)
# Forwards all command-line arguments to make.

param(
    [Parameter(ValueFromRemainingArguments = $true)]
    [string[]] $Args
)

# Join arguments safely and quote them for sh
$joinedArgs = $Args -join ' '
$cmd = "make $joinedArgs"

# Run inside sh -lc to get POSIX behavior
sh -lc "$cmd"