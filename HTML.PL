# -*-Perl-*-#
#
# html.p 
#
# Some functions which will provide html 
# features
#
# Copyright 1995, Brian C. Jepson
#                 Senior Programmer/Analyst, 
#                 Prosoft Systems International, Inc.
#
# This program is free software; you can redistribute it and/or modify
# it under the terms of the GNU General Public License as published by
# the Free Software Foundation; either version 2 of the License, or
# (at your option) any later version.
#
# This program is distributed in the hope that it will be useful,
# but WITHOUT ANY WARRANTY; without even the implied warranty of
# MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
# GNU General Public License for more details.
# 
# You should have received a copy of the GNU General Public License
# along with this program; if not, write to the Free Software
# Foundation, Inc., 675 Mass Ave, Cambridge, MA 02139, USA.
#
#

# generate an HTML header
#
sub html_head {

    local($title) = @_;
    print "Content-type: text/html\n\n";
    print "<HTML>\n\n";

    print "<TITLE>$title</TITLE>\n";
    print "<HEAD>\n";
    print "</HEAD>\n";
    print "<BODY>\n";
}

# generate an HTML footer
#
sub html_foot {
	print "</BODY>\n";
	print "\n</HTML>\n";
}

#
#
# puts a single line of text into a header format
#
#	&html_styleheader('1', 'I am a dingo');
#
# would result in the following html:
#
#	<H1>I am a Dingo</H1><p>
#
sub html_styleheader {

	print "<H$_[0]>$_[1]</H$_[0]><p>\n";

}

#
# generate an HREF
#
# &href("http://akira.psii.com/howdy.html", "Welcome to the moon, goon");
#
sub href {

	print "<A HREF=\"$_[0]\" > $_[1] </A><p>";
}

#
# generate a text input field
#
sub html_text_input {
    local($name, $size) = @_;
    print "<input type=\"text\" name=\"$name\" size=$size><p>\n";
}

#
# generate form header
#
sub html_form_start {
    local($action, $method) = @_;
    print "<form action=\"$action\" method=\"$method\">\n";
}

#
# generate form footer
#
sub html_form_end {
    print "</form>";
}


#
# generate a select list
#
sub html_select {
    local($name, *options) = @_;

    print "<select name=$name>\n";

    foreach $t (@options) {
	print "<option> $t\n";
    }

    print "</select> <P>\n";
}

#
# generate a radio button set
#
sub html_radio {

    local($name, *options) = @_;

    foreach $t (keys %options) {
	print "<input type=\"radio\" name=$name value=\"$t\" > $options{$t}<BR>\n";
    }

    print "<P>\n";
}

sub html_textarea {

    local($name, $rows, $cols) = @_;
    print "<textarea name=$name rows=$rows cols=$cols></textarea>";
}

sub html_button {
    local($action, $caption) = @_;
    print "<input type=\"$action\" value=\"$caption\">";
}
1;



