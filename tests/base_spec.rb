require_relative 'spec_helper'

# Check Vbox addtitions are installed

# Check user vagrant exists
# Check user vagrant is administrator
describe user('vagrant') do
  it { should exist }
  it { should belong_to_group('Administrators') }
end

# Check user unvagrant exists
# Check user unvagrant is not administrator and plain user
describe user('unvagrant') do
  it { should exist }
  it { should belong_to_group('Users') }
  it { should_not belong_to_group('Administrators') }
end

# Check UAC is on and UAC allows admin commands
describe windows_registry_key('HKEY_LOCAL_MACHINE\Software\Microsoft\Windows\CurrentVersion\Policies\System') do

  it { should exist }
  it { should have_property_value('EnableLUA', :type_qword, '1') }
  it { should have_property_value('ConsentPromptBehaviorAdmin', :type_qword, '0') }
  it { should have_property_value('ConsentPromptBehaviorUser', :type_qword, '0') }
  it { should have_property_value('PromptOnSecureDesktop', :type_qword, '0') }
end


# Check Chocolatey is installed
describe file('C:\\ProgramData\\chocolatey\\choco.exe') do
  it { should be_file }
end

describe command('choco') do
  its(:stdout) { should match /Chocolatey.*/ }
  #its(:stderr) { should match /stderr/ }
  its(:exit_status) { should eq 1 }
end

# Check doig is intalled and running
describe file('C:\\Windows\\doit.exe') do
  it { should be_file }
end
describe file('C:\\Windows\\doitclient.exe') do
  it { should be_file }
end
describe port(17481) do
  it { should be_listening }
end

# Check VM has internet access

# Check VM is activated

# Check c:\vagrant directory is accesible, writable

# Check Git is installed
describe command('git') do
  its(:stdout) { should match /usage: git.*/ }
  #its(:stderr) { should match /stderr/ }
  its(:exit_status) { should eq 1 }
end

