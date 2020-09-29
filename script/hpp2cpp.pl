#! /usr/bin/perl
# Generate cpp from head file that contain a bund of struct that represent a json schema.
# Usage:
# Provide file in cmdline argumen to generate jstruct.cpp
#   $ perl hpp2cpp.pl jstruct.h
# Read from STDIN, almost same but first line without #include "jstruct.h" line
#   $ cat jstruct.h | perl hpp2cpp.pl > jstruct.cpp
# Call from vim editor when select some lines, replace with the generated lines
#   :'<,'>!perl hpp2cpp.pl
#   

use strict;
use warnings;

my $DEBUG = 0;

my $hppfile = '';
my $cppfile = '';
my $namespace = '';

my $clsname = "";
my @fields = ();
my $clscount = 0;

my $fhin;
my $fhout;
if ($ARGV[0] && $ARGV[0] =~ /(.+)\.hp*$/i) {
	$hppfile = $ARGV[0];
	open($fhin, '<', $hppfile) or die "cannot open $hppfile $!";
	$cppfile = "$1.cpp";
	open($fhout, '>', $cppfile) or die "cannot open $cppfile $!";
}
else {
	$fhin = \*STDIN;
	$fhout = \*STDOUT;
}

while (<$fhin>) {
	chomp;
	if (/^\s*\{?\s*$/) {
		next
	}
	elsif (/^\s*namespace\s*(\w+)/) {
		$namespace = $1;
		&output_begin;
		next;
	}
	elsif (/^\s*struct\s*(\w+)/) {
		$clsname = $1;
		print STDERR "find struct $clsname\n" if $DEBUG;
		next;
	}
	elsif (/^\s*class\s*(\w+)/) {
		$clsname = $1;
		next;
	}
	elsif (/\};\s*$/) {
		print STDERR "find end of struct $clsname\n" if $DEBUG;
		 &output;
		 $clsname = "";
		@fields = ();
	}
	elsif ($clsname && $_ =~ /^\s*(\w[a-zA-Z0-9_:<>*]+)\s+(\w+)\s*;/) {
		my $type = $1;
		my $name = $2;
		my $line = $_;
		push(@fields, [$type, $name, $line]);
	}
}

print STDERR "generate for $clscount struct\n" if $DEBUG;
&output_end;

################################ sub functions ################################

# #include "header.h"
# namespance req {
sub output_begin
{
	if ($hppfile) {
		my $include = $hppfile;
		$include =~ s(^.*[/\\])(); # base file without leading path
		print $fhout qq{#include "$include"\n\n};
	}
	if ($namespace) {
		print $fhout "namespace $namespace\n\{\n";
	}
}

# } // endof namespace
sub output_end
{
	if ($namespace) {
		print $fhout "\} // END_NAMESPACE($namespace)\n";
	}
	if ($hppfile) {
		close($fhin);
	}
	if ($cppfile) {
		close($fhout);
	}
}

# output for each json struct, four methods
sub output
{
	print $fhout "/* ************************ $clsname ************************ */\n";

	print $fhout "\n";
	output_ctor($clsname, \@fields);
	print $fhout "\n";
	output_detor($clsname, \@fields);
	print $fhout "\n";
	output_LoadJson($clsname, \@fields);
	print $fhout "\n";
	output_SaveJson($clsname, \@fields);
	print $fhout "\n";

	$clscount++;
}

sub output_ctor
{
	my ($struct, $fields) = @_;

	print $fhout "${struct}::${struct}()\n";
	print $fhout "{\n";

	foreach my $field (@$fields) {
		my $type = $field->[0];
		my $name = $field->[1];

		# parse comment, // =default_val
		my $defVal = undef;
		my $line = $field->[2];
		if ($line =~ m|//\s*(.+)|) {
		    my $comment = $1;
		    if ($comment =~ /=\s*(.+)/) {
		        $defVal = $1;
		    }
		}

		my $initVal;
		if ($type eq "int") {
			$initVal = $defVal // "0";
		}
		elsif ($type eq "double") {
			$initVal = $defVal // "0.0";
		}
		elsif ($type eq "bool") {
			$initVal = $defVal // "false";
		}
		elsif ($type =~ /\*$/) {
			$initVal = "NULL";
		}
		elsif ($type eq "std::string") {
			$initVal = $defVal if $defVal;
		}
		else {
			# other type no need inited in ctor
		}

		if (defined $initVal) {
			print $fhout "\t$name = $initVal;\n";
		}
	}
	
	print $fhout "}\n";
}

sub output_detor
{
	my ($struct, $fields) = @_;

	print $fhout "${struct}::~${struct}()\n";
	print $fhout "{\n";

	foreach my $field (@$fields) {
		my $type = $field->[0];
		my $name = $field->[1];
		if ($type =~ /\*$/) {
			print $fhout "\tDELETE_OBJECT($name);\n";
		}
		elsif ($type =~ /vector<\w+\*>/) {
			print $fhout "\tfor (size_t i= 0; i < $name.size(); ++i)\n";
			print $fhout "\t{\n";
			print $fhout "\t\tDELETE_OBJECT(${name}[i]);\n";
			print $fhout "\t}\n";
		}
	}
	
	print $fhout "}\n";
}

sub output_LoadJson
{
	my ($struct, $fields) = @_;

	print $fhout "void ${struct}::LoadJson(LOADJSON_ARGUMENT_CPP)\n";
	print $fhout "{\n";

	foreach my $field (@$fields) {
		my $type = $field->[0];
		my $name = $field->[1];
		my $jsname = $name;
		my $flag = 0;
		if ($jsname =~ /(int|class)_/) {
			$jsname = $1;
			$flag = 1;
		}
		if ($type =~ /\*$/) {
			# print "\textract_json_value(&$name, json, path, \"$jsname\", callback);\n";
			if ($flag == 0) {
				print $fhout "\tEXTRACT_JSON_OBJECT($name);\n";
			}
			else {
				print $fhout "\tEXTRACT_JSON_OBJECT2($name, \"$jsname\");\n";
			}
		}
		else{
			# print "\textract_json_value($name, json, path, \"$jsname\", callback);\n";
			if ($flag == 0) {
				print $fhout "\tEXTRACT_JSON_VALUE($name);\n";
			}
			else {
				print $fhout "\tEXTRACT_JSON_VALUE2($name, \"$jsname\");\n";
			}
		}
	}
	
	print $fhout "}\n";
}

sub output_SaveJson
{
	my ($struct, $fields) = @_;

	print $fhout "void ${struct}::SaveJson(SAVEJSON_ARGUMENT_CPP)\n";
	print $fhout "{\n";

	foreach my $field (@$fields) {
		my $type = $field->[0];
		my $name = $field->[1];
		my $line = $field->[2];
		if ($line =~ m|//\s*(.+)|) {
		    my $comment = $1;
		    if ($comment =~ /\bignore\b/i) {
			next; # ignore save this field
		    }
		}

		my $jsname = $name;
		my $flag = 0;
		if ($jsname =~ /(int|class)_/) {
			$jsname = $1;
			$flag = 1;
		}

		# print "\tinject_json_value($name, json, path, \"$jsname\", allocator, callback);\n";
		if ($flag == 0) {
			print $fhout "\tINJECT_JSON_VALUE($name);\n";
		}
		else {
			print $fhout "\tINJECT_JSON_VALUE2($name, \"$jsname\");\n";
		}
	}
	
	print $fhout "}\n";
}
