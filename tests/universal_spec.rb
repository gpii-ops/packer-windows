require_relative 'base_spec'

# Check IE browser don't have first run
describe windows_registry_key('HKEY_LOCAL_MACHINE\Software\Policies\Microsoft\Internet Explorer\Main') do
  it { should exist }
  it { should have_property_value('DisableFirstRunCustomize', :type_qword, '1') }
end

# Check UAC is on and UAC allows admin commands
describe windows_registry_key('HKEY_LOCAL_MACHINE\Software\Microsoft\Windows\CurrentVersion\Policies\System') do
  it { should exist }
  it { should have_property_value('EnableLUA', :type_dword, '1') }
  it { should have_property_value('ConsentPromptBehaviorAdmin', :type_dword, '0') }
  it { should have_property_value('ConsentPromptBehaviorUser', :type_dword, '0') }
  it { should have_property_value('PromptOnSecureDesktop', :type_dword, '0') }
end

# Check Firefox don't have first run
describe file('C:\\Program Files\\Mozilla Firefox\\defaults\\pref\\local-settings.js') do
  it { should be_file }
end
