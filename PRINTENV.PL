print "Content-type: text/html\n";
print "\n<HTML>\n\n";

print "<TITLE>$title</TITLE>";
print "<HEAD>\n\n";
print "</HEAD>\n\n";
print "<PRE>";

foreach $item (keys %ENV) {
    print "$item = $ENV{$item}\n";
}

print "</PRE>";
print "</HTML>";
