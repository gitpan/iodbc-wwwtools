#
# html.pm
#
# A "page-oriented" class library for creating html pages.
#
# The page is instantiated as an object, and html objects are
# added to the page, using its addobject method.
#
# Copyright (C) 1995, Brian C. Jepson
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
#### package: html
# This is a container class, which is further
# subclassed to various html objects.
####
package html;

#### method: new
#
####
sub new {
    my $self = {};
    bless $self;
    $self;
}    

#### method: addobject
# This is a method which allows html to behave as a container
# class. This method will add an object to the specified object's
# object array, and return a reference to it.
####
sub addobject {
    my $self = shift;
    my $newobj = shift;

    $self->{'objectcount'}++;	# increment the count of objects

    # call the new method for the requested object. Note that
    # @_ is passed on the the new object. Beware, however, that
    # the first element in that method's @_ is going to be
    # the value of $newobject
    #
    $self->{'objects'}[$self->{'objectcount'}] = eval('new '. $newobj . '@_');

    # return a reference to the object
    return $self->{'objects'}[$self->{'objectcount'}];
}

#### Method: show
# Cause the contained objects to be displayed
#
####
sub show {
    my $self = shift;
    my $count = $self->{'objectcount'};
    my $i;

    for ($i = 1;$i <= $count; $i++) {
	show  {$self->{'objects'}[$i]};
    }
}

# this is the htmlobject class. It is derived from html, and in turn,
# most of the html objects are derived from it.
#
package htmlobject;
@ISA= qw( html );
@htmlobject::Inherit::ISA = @ISA; # access to overridden methods
sub new {
    my $self = new html;
    bless $self;
    $self;
}    

# add an option to the tag
sub addopt {
    my $self = shift;
    my ($option, $value) = @_;
    $self->{'options'}{$option} = $value;
}

#### Method: show
#
# Although the superclass method is overriden here, I need to
# call the superclass' show method in order to show any contained
# objects. This will come in handy with forms and their objects
#
####
sub show {
    my $self = shift;

    if ($self->{'tag'} ne '') {
	print  "<$self->{'tag'}";

	foreach $key ( keys %{$self->{'options'}} ) {
	    print " $key=$self->{'options'}{$key}";
	}
	print ">";
    }
    $self->htmlobject::Inherit::show();
    print "$self->{'text'}";
    
    print "</$self->{'tag'}>\n" unless $self->{'tag'} eq '' ;

}

sub settag {
    my $self = shift;
    $self->{'tag'} = shift;
}

sub settext {
    my $self = shift;
    $self->{'text'} = shift;
}

package htmlpage;
@ISA= qw( html);
@htmlpage::Inherit::ISA = @ISA; # access to overridden methods
sub new {
    my $self = new html;
    bless $self;
    $self;
    
}

sub show {
    my $self = shift;

    print "Content-type: text/html\n\n";
    print "<HTML>\n\n";    
    print "<HEAD>\n\n";
    print "</HEAD>\n\n";

    $self->htmlpage::Inherit::show();
    print "</HTML>";
}

package html_link;
@ISA= qw( htmlobject html);
sub new {
    my $self = new htmlobject;
    bless $self;

    shift;			# pop off the first parameter...
    my ($linkto, $text) = @_;

    settag $self "A";
    addopt $self ("HREF", $linkto);
    settext $self $text;

    $self;
}    

package html_header;
@ISA= qw( htmlobject html);
sub new {
    my $self = new htmlobject;
    bless $self;

    shift;			# pop off the first parameter...
    my ($size, $text) = @_;

    settag $self "H$size";
    settext $self $text;

    $self;
}  

package html_text;
@ISA= qw( htmlobject html);
sub new {
    my $self = new htmlobject;
    bless $self;

    shift;			# pop off the first parameter...
    my ($text) = @_;

    settext $self $text;

    $self;
}  

package html_form;
@ISA = qw( htmlobject html);
sub new {
    my $self = new htmlobject;
    bless $self;

    shift;
    my ($action, $method, $enctype) = @_;

    $self->settag("FORM");
    $self->addopt("ACTION", $action);
    $self->addopt("METHOD", $method) unless $method eq '';
    $self->addopt("ENCTYPE", $enctype) unless $enctype eq '';
    $self;
}

package html_form_submit;
@ISA = qw( htmlobject html);
sub new {
    my $self = new htmlobject;
    bless $self;

    shift;
    my ($name) = @_;

    $self->settag("INPUT");
    $self->addopt("TYPE", "SUBMIT");
    $self->addopt("NAME", $name) unless $name eq '';
    $self;
}

package html_form_text;
@ISA = qw( htmlobject html);
sub new {
    my $self = new htmlobject;
    bless $self;
    
    shift;
    my ($name, $size, $value) = @_;

    $self->settag("INPUT");
    $self->addopt("TYPE", "TEXT");
    $self->addopt("NAME", $name) unless $name eq '';
    $self->addopt("SIZE", $size) unless $size eq '';
    $self->addopt("VALUE", $value) unless $value eq '';
    $self;
}

package html_form_hidden;
@ISA = qw( htmlobject html);
sub new {
    my $self = new htmlobject;
    bless $self;
    
    shift;
    my ($name, $value) = @_;

    $self->settag("INPUT");
    $self->addopt("TYPE", "TEXT");
    $self->addopt("VALUE", $value);
    $self->addopt("NAME", $name) unless $name eq '';

    $self;
}
1;


package html_table;
@ISA = qw( htmlobject html);
sub new {
    my $self = new htmlobject;
    bless $self;
    
    shift;
    local (*array) = @_;

    @{$self->{'array'}} = @array;
    $self;
}

sub show {
    my $self = shift;

    print "<table border>\n";

    foreach $header (keys %{$self->{'array'}[1]}) {
		print "<th>$header</th>"; 
	}
    foreach $record ( @{$self->{'array'}}) {
		print "<tr>\n   ";
		foreach $key (keys %{$record}) {
			print "<td>${$record}{$key}</td>";
		}
		print "\n</tr>\n";
	}
   	print "</table>\n";

}

1;

