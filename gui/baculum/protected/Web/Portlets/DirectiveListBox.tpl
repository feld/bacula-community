<div class="line<%=!$this->display_directive ? ' hide' : '';%>">
	<div class="text"><com:TActiveLabel ID="Label" ActiveControl.EnableUpdate="false" Visible="<%=$this->display_directive%>" />:</div>
	<div class="field directive_value">
		<com:TActiveListBox ID="Directive"
			SelectionMode="Multiple"
			OnSelectedIndexChanged="saveValue"
			CssClass="ftype_listbox"
			Visible="<%=$this->display_directive%>"
			AutoPostBack="false"
			ActiveControl.EnableUpdate="false"
		/> <%=$this->getRequired() ? '*' : ''%>
		<img src="<%=$this->getPage()->getTheme()->getBaseUrl()%>/switch.png" class="reset_btn" onclick="document.getElementById('<%=$this->Directive->ClientID%>').value = '<%=$this->getDefaultValue() === 0 ? '' : $this->getDefaultValue()%>';" alt="<%[ Reset to default value ]%>" title="<%[ Reset to default value ]%>" />
		<img src="<%=$this->getPage()->getTheme()->getBaseUrl()%>/icon_err.png" class="remove_btn" onclick="document.getElementById('<%=$this->Directive->ClientID%>').value = '';" alt="<%[ Remove directive ]%>" title="<%[ Remove directive ]%>" />
		<com:TRequiredFieldValidator
			ID="DirectiveValidator"
			ValidationGroup="Directive"
			Display="Dynamic"
			ControlToValidate="Directive"
			FocusOnError="true"
			Text="Field required."
			Enabled="<%=$this->getRequired() && $this->getShow()%>"
		/>
		<p class="smalldesc"><%[ Use Ctrl + Mouse Click to change selection ]%></p>
	</div>
</div>